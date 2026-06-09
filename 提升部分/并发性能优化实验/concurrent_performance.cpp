#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std;

using Histogram = vector<unsigned long long>;
using Clock = chrono::steady_clock;

struct Config {
    size_t itemCount = 600000;
    int threadCount = max(2u, thread::hardware_concurrency());
    int bucketCount = 4096;
    int hotPercent = 70;
    int hotBuckets = 16;
    int heavyPercent = 20;
    int heavyCost = 40;
    int chunkSize = 4096;
};

struct Workload {
    vector<uint32_t> keys;
    vector<uint16_t> costs;
    int bucketCount = 0;
};

struct BenchResult {
    string name;
    double milliseconds = 0.0;
    double throughput = 0.0;
    double speedup = 0.0;
    unsigned long long checksum = 0;
    bool correct = true;
};

uint32_t mix32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

uint32_t computeBucket(uint32_t key, uint16_t cost, int bucketCount) {
    uint32_t value = key;
    for (uint16_t i = 0; i < cost; ++i) {
        value = mix32(value + 0x9e3779b9U + i);
    }
    return value % static_cast<uint32_t>(bucketCount);
}

unsigned long long checksumOf(const Histogram& histogram) {
    unsigned long long sum = 0;
    for (unsigned long long value : histogram) {
        sum += value;
    }
    return sum;
}

bool sameHistogram(const Histogram& a, const Histogram& b) {
    return a.size() == b.size() && equal(a.begin(), a.end(), b.begin());
}

Workload makeContentionWorkload(const Config& cfg) {
    Workload workload;
    workload.bucketCount = cfg.bucketCount;
    workload.keys.resize(cfg.itemCount);
    workload.costs.assign(cfg.itemCount, 0);

    mt19937 rng(20260608);
    uniform_int_distribution<int> percentDist(0, 99);
    uniform_int_distribution<int> hotDist(0, max(0, cfg.hotBuckets - 1));
    uniform_int_distribution<int> allDist(0, cfg.bucketCount - 1);

    for (size_t i = 0; i < cfg.itemCount; ++i) {
        if (percentDist(rng) < cfg.hotPercent) {
            workload.keys[i] = static_cast<uint32_t>(hotDist(rng));
        } else {
            workload.keys[i] = static_cast<uint32_t>(allDist(rng));
        }
    }
    return workload;
}

Workload makeImbalancedWorkload(const Config& cfg) {
    Workload workload;
    workload.bucketCount = cfg.bucketCount;
    workload.keys.resize(cfg.itemCount);
    workload.costs.assign(cfg.itemCount, 1);

    mt19937 rng(20260609);
    uniform_int_distribution<int> keyDist(0, cfg.bucketCount - 1);
    const size_t heavyEnd = cfg.itemCount * static_cast<size_t>(cfg.heavyPercent) / 100;

    for (size_t i = 0; i < cfg.itemCount; ++i) {
        workload.keys[i] = static_cast<uint32_t>(keyDist(rng));
        if (i < heavyEnd) {
            workload.costs[i] = static_cast<uint16_t>(cfg.heavyCost);
        }
    }
    return workload;
}

Histogram serialHistogram(const Workload& workload) {
    Histogram histogram(workload.bucketCount, 0);
    for (size_t i = 0; i < workload.keys.size(); ++i) {
        ++histogram[computeBucket(workload.keys[i], workload.costs[i], workload.bucketCount)];
    }
    return histogram;
}

Histogram coarseMutexHistogram(const Workload& workload, int threadCount) {
    Histogram histogram(workload.bucketCount, 0);
    mutex histogramMutex;
    vector<thread> workers;

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            const size_t begin = workload.keys.size() * static_cast<size_t>(t) / threadCount;
            const size_t end = workload.keys.size() * static_cast<size_t>(t + 1) / threadCount;
            for (size_t i = begin; i < end; ++i) {
                uint32_t bucket = computeBucket(workload.keys[i], workload.costs[i], workload.bucketCount);
                lock_guard<mutex> lock(histogramMutex);
                ++histogram[bucket];
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
    return histogram;
}

Histogram bucketLockHistogram(const Workload& workload, int threadCount) {
    Histogram histogram(workload.bucketCount, 0);
    vector<mutex> bucketLocks(workload.bucketCount);
    vector<thread> workers;

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            const size_t begin = workload.keys.size() * static_cast<size_t>(t) / threadCount;
            const size_t end = workload.keys.size() * static_cast<size_t>(t + 1) / threadCount;
            for (size_t i = begin; i < end; ++i) {
                uint32_t bucket = computeBucket(workload.keys[i], workload.costs[i], workload.bucketCount);
                lock_guard<mutex> lock(bucketLocks[bucket]);
                ++histogram[bucket];
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
    return histogram;
}

Histogram atomicHistogram(const Workload& workload, int threadCount) {
    vector<atomic<unsigned long long>> counters(workload.bucketCount);
    for (auto& counter : counters) {
        counter.store(0, memory_order_relaxed);
    }

    vector<thread> workers;
    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            const size_t begin = workload.keys.size() * static_cast<size_t>(t) / threadCount;
            const size_t end = workload.keys.size() * static_cast<size_t>(t + 1) / threadCount;
            for (size_t i = begin; i < end; ++i) {
                uint32_t bucket = computeBucket(workload.keys[i], workload.costs[i], workload.bucketCount);
                counters[bucket].fetch_add(1, memory_order_relaxed);
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    Histogram histogram(workload.bucketCount, 0);
    for (int i = 0; i < workload.bucketCount; ++i) {
        histogram[i] = counters[i].load(memory_order_relaxed);
    }
    return histogram;
}

Histogram localStaticHistogram(const Workload& workload, int threadCount) {
    vector<Histogram> locals(threadCount, Histogram(workload.bucketCount, 0));
    vector<thread> workers;

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            const size_t begin = workload.keys.size() * static_cast<size_t>(t) / threadCount;
            const size_t end = workload.keys.size() * static_cast<size_t>(t + 1) / threadCount;
            Histogram& local = locals[t];
            for (size_t i = begin; i < end; ++i) {
                ++local[computeBucket(workload.keys[i], workload.costs[i], workload.bucketCount)];
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    Histogram histogram(workload.bucketCount, 0);
    for (const auto& local : locals) {
        for (int i = 0; i < workload.bucketCount; ++i) {
            histogram[i] += local[i];
        }
    }
    return histogram;
}

Histogram localDynamicHistogram(const Workload& workload, int threadCount, int chunkSize) {
    vector<Histogram> locals(threadCount, Histogram(workload.bucketCount, 0));
    atomic<size_t> nextIndex(0);
    vector<thread> workers;
    const size_t chunk = static_cast<size_t>(max(1, chunkSize));

    for (int t = 0; t < threadCount; ++t) {
        workers.emplace_back([&, t]() {
            Histogram& local = locals[t];
            while (true) {
                const size_t begin = nextIndex.fetch_add(chunk, memory_order_relaxed);
                if (begin >= workload.keys.size()) {
                    break;
                }
                const size_t end = min(begin + chunk, workload.keys.size());
                for (size_t i = begin; i < end; ++i) {
                    ++local[computeBucket(workload.keys[i], workload.costs[i], workload.bucketCount)];
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }

    Histogram histogram(workload.bucketCount, 0);
    for (const auto& local : locals) {
        for (int i = 0; i < workload.bucketCount; ++i) {
            histogram[i] += local[i];
        }
    }
    return histogram;
}

template <typename Func>
BenchResult measure(const string& name,
                    const Workload& workload,
                    const Histogram& reference,
                    double baseMilliseconds,
                    Func func) {
    const auto start = Clock::now();
    Histogram histogram = func();
    const auto finish = Clock::now();
    const double ms = chrono::duration<double, milli>(finish - start).count();

    BenchResult result;
    result.name = name;
    result.milliseconds = ms;
    result.throughput = static_cast<double>(workload.keys.size()) / ms / 1000.0;
    result.speedup = baseMilliseconds / ms;
    result.checksum = checksumOf(histogram);
    result.correct = sameHistogram(histogram, reference);
    return result;
}

void printResults(const vector<BenchResult>& results) {
    cout << left << setw(24) << "方案"
         << right << setw(14) << "耗时(ms)"
         << setw(18) << "吞吐(Mops/s)"
         << setw(12) << "加速比"
         << setw(14) << "校验"
         << "\n";
    cout << string(82, '-') << "\n";

    cout << fixed << setprecision(3);
    for (const auto& result : results) {
        cout << left << setw(24) << result.name
             << right << setw(14) << result.milliseconds
             << setw(18) << result.throughput
             << setw(12) << result.speedup
             << setw(14) << (result.correct ? "OK" : "ERROR")
             << "\n";
    }
}

void runContentionExperiment(const Config& cfg) {
    cout << "\n实验一：共享计数器并发优化（热点比例 " << cfg.hotPercent << "%）\n";
    Workload workload = makeContentionWorkload(cfg);

    const auto baseStart = Clock::now();
    Histogram reference = serialHistogram(workload);
    const auto baseFinish = Clock::now();
    const double baseMs = chrono::duration<double, milli>(baseFinish - baseStart).count();

    vector<BenchResult> results;
    results.push_back({"单线程基准", baseMs,
                       static_cast<double>(workload.keys.size()) / baseMs / 1000.0,
                       1.0, checksumOf(reference), true});
    results.push_back(measure("粗粒度互斥锁", workload, reference, baseMs, [&]() {
        return coarseMutexHistogram(workload, cfg.threadCount);
    }));
    results.push_back(measure("桶级细粒度锁", workload, reference, baseMs, [&]() {
        return bucketLockHistogram(workload, cfg.threadCount);
    }));
    results.push_back(measure("原子计数器", workload, reference, baseMs, [&]() {
        return atomicHistogram(workload, cfg.threadCount);
    }));
    results.push_back(measure("线程本地归约", workload, reference, baseMs, [&]() {
        return localStaticHistogram(workload, cfg.threadCount);
    }));

    printResults(results);
}

void runLoadBalanceExperiment(const Config& cfg) {
    cout << "\n实验二：负载不均衡下的任务调度优化（前 "
         << cfg.heavyPercent << "% 数据为重任务）\n";
    Workload workload = makeImbalancedWorkload(cfg);

    const auto baseStart = Clock::now();
    Histogram reference = serialHistogram(workload);
    const auto baseFinish = Clock::now();
    const double baseMs = chrono::duration<double, milli>(baseFinish - baseStart).count();

    vector<BenchResult> results;
    results.push_back({"单线程基准", baseMs,
                       static_cast<double>(workload.keys.size()) / baseMs / 1000.0,
                       1.0, checksumOf(reference), true});
    results.push_back(measure("静态连续划分", workload, reference, baseMs, [&]() {
        return localStaticHistogram(workload, cfg.threadCount);
    }));
    results.push_back(measure("动态块调度", workload, reference, baseMs, [&]() {
        return localDynamicHistogram(workload, cfg.threadCount, cfg.chunkSize);
    }));

    printResults(results);
}

void printConfig(const Config& cfg) {
    cout << "\n当前参数："
         << "数据量=" << cfg.itemCount
         << "，线程数=" << cfg.threadCount
         << "，桶数=" << cfg.bucketCount
         << "，热点比例=" << cfg.hotPercent << "%"
         << "，重任务比例=" << cfg.heavyPercent << "%"
         << "，动态块大小=" << cfg.chunkSize
         << "\n";
}

Config readCustomConfig() {
    Config cfg;
    cout << "输入数据量 线程数 桶数: ";
    cin >> cfg.itemCount >> cfg.threadCount >> cfg.bucketCount;
    cout << "输入热点比例(0-100) 热点桶数: ";
    cin >> cfg.hotPercent >> cfg.hotBuckets;
    cout << "输入重任务比例(0-100) 重任务计算成本 动态块大小: ";
    cin >> cfg.heavyPercent >> cfg.heavyCost >> cfg.chunkSize;

    cfg.threadCount = max(1, cfg.threadCount);
    cfg.bucketCount = max(1, cfg.bucketCount);
    cfg.hotPercent = min(100, max(0, cfg.hotPercent));
    cfg.hotBuckets = min(cfg.bucketCount, max(1, cfg.hotBuckets));
    cfg.heavyPercent = min(100, max(0, cfg.heavyPercent));
    cfg.heavyCost = max(1, cfg.heavyCost);
    cfg.chunkSize = max(1, cfg.chunkSize);
    return cfg;
}

int main() {
    cout << "自由扩展提升：并发性能优化实验\n";
    cout << "本程序对比锁粒度、原子操作、线程本地归约和动态任务调度的性能差异。\n";

    Config cfg;
    while (true) {
        cout << "\n1. 使用默认参数运行\n";
        cout << "2. 自定义参数运行\n";
        cout << "0. 退出\n";
        cout << "请输入选项: ";

        int choice;
        if (!(cin >> choice)) {
            return 0;
        }
        if (choice == 0) {
            break;
        }
        if (choice == 1) {
            cfg = Config();
        } else if (choice == 2) {
            cfg = readCustomConfig();
        } else {
            cout << "无效选项。\n";
            continue;
        }

        printConfig(cfg);
        runContentionExperiment(cfg);
        runLoadBalanceExperiment(cfg);
    }

    return 0;
}
