#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "sync.h"

// 清空输入缓冲区
static void sync_clear_input_buffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

// 一、生产者—消费者问题
#define PC_BUFFER_SIZE 5 // 缓冲区大小，表示最多可以同时存放 5 个产品
#define PC_PRODUCER_COUNT 2 // 生产者线程数量
#define PC_CONSUMER_COUNT 2 // 消费者线程数量
#define PC_ITEM_PER_PRODUCER 5 // 每个生产者需要生产的产品数量

static int pc_buffer[PC_BUFFER_SIZE]; // 生产者消费者问题中的共享缓冲区，用于存放生产出来的产品
static int pc_in = 0; // 缓冲区写入位置下标
static int pc_out = 0; // 缓冲区读取位置下标
static int pc_count = 0; // 当前缓冲区中的产品数量

static sem_t pc_empty; // 表示空缓冲区数量
static sem_t pc_full; // 表示已有产品数量
static pthread_mutex_t pc_mutex; // 互斥锁 保护缓冲区

// 打印生产者消费者问题中当前缓冲区的状态
static void print_pc_buffer()
{
    int i;

    printf("当前缓冲区：");
    for (i = 0; i < PC_BUFFER_SIZE; i++)
    {
        printf("[%d] ", pc_buffer[i]);
    }
    printf("  当前产品数：%d\n", pc_count);
}

// 生产者线程函数
static void* producer_thread(void* arg)
{
    int producer_id = *(int*)arg; // 当前生产者编号
    int i;
    int item; // 当前生产者本次生产出来的产品编号

    for (i = 1; i <= PC_ITEM_PER_PRODUCER; i++)
    {
        item = producer_id * 100 + i;

        // 申请一个空缓冲区位置
        // empty > 0 才能生产，否则说明缓冲区已满
        sem_wait(&pc_empty);

        // 加锁进入临界区，互斥访问缓冲区
        pthread_mutex_lock(&pc_mutex);

        // 将产品放入 pc_in 指向的缓冲区位置
        pc_buffer[pc_in] = item;
        pc_in = (pc_in + 1) % PC_BUFFER_SIZE;
        pc_count++;

        printf("生产者 P%d 生产产品 %d。\n", producer_id, item);
        print_pc_buffer();

        // 解锁离开临界区
        pthread_mutex_unlock(&pc_mutex);

        // 产品数量增加，唤醒可能等待的消费者
        sem_post(&pc_full);

        sleep(1);
    }

    return NULL;
}

// 消费者线程函数
static void* consumer_thread(void* arg)
{
    int consumer_id = *(int*)arg; // 当前消费者编号
    int i;
    int item; // 当前消费者本次取出的产品编号

    for (i = 1; i <= PC_ITEM_PER_PRODUCER; i++)
    {
        // 申请一个已有产品
        // full > 0 才能消费，否则说明缓冲区为空
        sem_wait(&pc_full);

        // 加锁进入临界区，互斥访问缓冲区
        pthread_mutex_lock(&pc_mutex);

        item = pc_buffer[pc_out];
        pc_buffer[pc_out] = 0;
        pc_out = (pc_out + 1) % PC_BUFFER_SIZE;
        pc_count--;

        printf("消费者 C%d 消费产品 %d。\n", consumer_id, item);
        print_pc_buffer();

        // 解锁离开临界区
        pthread_mutex_unlock(&pc_mutex);

        // 空缓冲区数量增加，唤醒可能等待的生产者
        sem_post(&pc_empty);

        sleep(1);
    }

    return NULL;
}

// 生产者消费者问题模拟
static void run_producer_consumer()
{
    pthread_t producers[PC_PRODUCER_COUNT]; // 生产者线程数组
    pthread_t consumers[PC_CONSUMER_COUNT]; // 消费者线程数组
    int producer_ids[PC_PRODUCER_COUNT]; // 生产者编号数组
    int consumer_ids[PC_CONSUMER_COUNT]; // 消费者编号数组
    int i;

    pc_in = 0; // 初始化循环缓冲区的写入下标
    pc_out = 0;  // 初始化循环缓冲区的读取下标
    pc_count = 0; // 初始化当前产品数量

    // 初始化缓冲区内容，0 表示当前位置暂时没有产品
    for (i = 0; i < PC_BUFFER_SIZE; i++)
    {
        pc_buffer[i] = 0;
    }

    sem_init(&pc_empty, 0, PC_BUFFER_SIZE);
    sem_init(&pc_full, 0, 0);
    // 初始化互斥锁，用于保护缓冲区共享数据
    pthread_mutex_init(&pc_mutex, NULL);

    printf("\n========== 生产者—消费者问题 ==========\n");
    printf("缓冲区大小：%d\n", PC_BUFFER_SIZE);
    printf("生产者数量：%d，每个生产者生产 %d 个产品\n",
        PC_PRODUCER_COUNT, PC_ITEM_PER_PRODUCER);
    printf("消费者数量：%d，每个消费者消费 %d 个产品\n",
        PC_CONSUMER_COUNT, PC_ITEM_PER_PRODUCER);
    printf("--------------------------------------\n");
    // 创建生产者线程
    for (i = 0; i < PC_PRODUCER_COUNT; i++)
    {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer_thread, &producer_ids[i]);
    }
    // 创建消费者线程
    for (i = 0; i < PC_CONSUMER_COUNT; i++)
    {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer_thread, &consumer_ids[i]);
    }
    // 等待所有生产者线程执行结束
    for (i = 0; i < PC_PRODUCER_COUNT; i++)
    {
        pthread_join(producers[i], NULL);
    }
    // 等待所有消费者线程执行结束
    for (i = 0; i < PC_CONSUMER_COUNT; i++)
    {
        pthread_join(consumers[i], NULL);
    }

    pthread_mutex_destroy(&pc_mutex); // 销毁互斥锁，释放相关资源
    sem_destroy(&pc_empty); // 销毁空缓冲区信号量
    sem_destroy(&pc_full);  // 销毁已有产品信号量

    printf("--------------------------------------\n");
    printf("生产者—消费者问题模拟结束。\n");

}

// 二、读者—写者问题
#define RW_READER_COUNT 5 // 读者线程数量
#define RW_WRITER_COUNT 2 // 写者线程数量

static int rw_data = 0; // 读者写者问题中的共享数据
static int rw_reader_count = 0; // 当前正在读取共享数据的读者数量

static pthread_mutex_t rw_data_mutex; // 保护共享数据 rw_data 的互斥锁，写者写入时需要独占该锁
static pthread_mutex_t rw_reader_count_mutex; // 保护读者数量 rw_reader_count 的互斥锁，防止多个读者同时修改读者计数

// 读者线程函数
static void* reader_thread(void* arg)
{
    int reader_id = *(int*)arg; // 当前读者编号

    // 修改读者数量前先加锁
    pthread_mutex_lock(&rw_reader_count_mutex);
    rw_reader_count++;

    // 第一个读者进入时，锁住共享数据
    if (rw_reader_count == 1)
    {
        pthread_mutex_lock(&rw_data_mutex);
    }

    printf("读者 R%d 进入，当前读者数量：%d。\n",
        reader_id, rw_reader_count);

    // 读者数量修改完成，释放读者计数锁
    pthread_mutex_unlock(&rw_reader_count_mutex);

    // 读操作区
    printf("读者 R%d 正在读取共享数据：%d。\n", reader_id, rw_data);
    sleep(1);

    // 读者准备离开，需要修改读者数量，因此先加锁
    pthread_mutex_lock(&rw_reader_count_mutex);

    rw_reader_count--;
    printf("读者 R%d 离开，当前读者数量：%d。\n",
        reader_id, rw_reader_count);

    // 最后一个读者离开时，释放共享数据，写者才可以进入。
    if (rw_reader_count == 0)
    {
        pthread_mutex_unlock(&rw_data_mutex);
    }
    // 释放读者计数锁
    pthread_mutex_unlock(&rw_reader_count_mutex);

    return NULL;
}

// 写者线程函数
static void* writer_thread(void* arg)
{
    int writer_id = *(int*)arg; // 当前写者编号

    // 写者必须独占共享数据
    // 如果有读者正在读，或者其他写者正在写，这里会阻塞等待
    pthread_mutex_lock(&rw_data_mutex);

    printf("写者 W%d 进入，准备写入共享数据。\n", writer_id);

    rw_data += writer_id * 10;
    printf("写者 W%d 写入完成，当前共享数据：%d。\n",
        writer_id, rw_data);

    sleep(1);

    printf("写者 W%d 离开。\n", writer_id);

    // 写操作完成后释放共享数据锁
    pthread_mutex_unlock(&rw_data_mutex);

    return NULL;
}

// 读者写者问题模拟
static void run_readers_writers()
{
    pthread_t readers[RW_READER_COUNT]; // 读者线程数组
    pthread_t writers[RW_WRITER_COUNT]; // 写者线程数组
    int reader_ids[RW_READER_COUNT]; // 读者编号数组
    int writer_ids[RW_WRITER_COUNT]; // 写者编号数组
    int i;

    rw_data = 0; // 初始化共享数据
    rw_reader_count = 0; // 初始化当前读者数量

    pthread_mutex_init(&rw_data_mutex, NULL); // 初始化共享数据互斥锁
    pthread_mutex_init(&rw_reader_count_mutex, NULL); // 初始化读者计数互斥锁

    printf("\n========== 读者—写者问题 ==========\n");
    printf("读者数量：%d\n", RW_READER_COUNT);
    printf("写者数量：%d\n", RW_WRITER_COUNT);
    printf("策略：读者优先，多个读者可同时读，写者写时独占访问。\n");
    printf("----------------------------------\n");

    for (i = 0; i < RW_READER_COUNT; i++)
    {
        reader_ids[i] = i + 1;
    }

    for (i = 0; i < RW_WRITER_COUNT; i++)
    {
        writer_ids[i] = i + 1;
    }

    // 按固定顺序创建读者和写者线程
    pthread_create(&readers[0], NULL, reader_thread, &reader_ids[0]);
    pthread_create(&readers[1], NULL, reader_thread, &reader_ids[1]);
    pthread_create(&writers[0], NULL, writer_thread, &writer_ids[0]);
    pthread_create(&readers[2], NULL, reader_thread, &reader_ids[2]);
    pthread_create(&writers[1], NULL, writer_thread, &writer_ids[1]);
    pthread_create(&readers[3], NULL, reader_thread, &reader_ids[3]);
    pthread_create(&readers[4], NULL, reader_thread, &reader_ids[4]);

    // 等待所有读者线程执行结束
    for (i = 0; i < RW_READER_COUNT; i++)
    {
        pthread_join(readers[i], NULL);
    }
    // 等待所有写者线程执行结束
    for (i = 0; i < RW_WRITER_COUNT; i++)
    {
        pthread_join(writers[i], NULL);
    }

    pthread_mutex_destroy(&rw_data_mutex); // 销毁共享数据互斥锁
    pthread_mutex_destroy(&rw_reader_count_mutex);  // 销毁读者计数互斥锁

    printf("----------------------------------\n");
    printf("读者—写者问题模拟结束。\n");
}

// 三、哲学家进餐问题
#define PHILOSOPHER_COUNT 5 // 哲学家数量
#define PHILOSOPHER_EAT_TIMES 3// 每位哲学家需要进餐的次数

static pthread_mutex_t forks[PHILOSOPHER_COUNT]; // 筷子互斥锁数组，每一把筷子对应一个互斥锁
static sem_t room; // 房间信号量

// 哲学家线程函数
static void* philosopher_thread(void* arg)
{
    int id = *(int*)arg; // 当前哲学家的编号
    int left_fork = id; // 当前哲学家左边筷子的编号
    int right_fork = (id + 1) % PHILOSOPHER_COUNT; // 当前哲学家右边筷子的编号
    int i;

    for (i = 1; i <= PHILOSOPHER_EAT_TIMES; i++)
    {
        printf("哲学家 %d 正在思考。\n", id + 1);
        sleep(1);

        // room 最多允许 4 个哲学家同时尝试拿筷子，避免 5 个哲学家同时占有一只筷子造成死锁。
        sem_wait(&room);

        printf("哲学家 %d 尝试拿起筷子。\n", id + 1);
        // 拿起左筷子，对应锁住左筷子的互斥锁
        pthread_mutex_lock(&forks[left_fork]);
        printf("哲学家 %d 拿起左筷子 %d。\n", id + 1, left_fork);
        // 拿起右筷子，对应锁住右筷子的互斥锁
        pthread_mutex_lock(&forks[right_fork]);
        printf("哲学家 %d 拿起右筷子 %d。\n", id + 1, right_fork);
        // 同时拿到左右两只筷子后，哲学家开始进餐
        printf("哲学家 %d 正在进餐，第 %d 次进餐。\n", id + 1, i);
        // 暂停 1 秒，用于模拟进餐过程
        sleep(1);
        // 放下右筷子，也就是释放右筷子的互斥锁
        pthread_mutex_unlock(&forks[right_fork]);
        printf("哲学家 %d 放下右筷子 %d。\n", id + 1, right_fork);
        // 放下左筷子，也就是释放左筷子的互斥锁
        pthread_mutex_unlock(&forks[left_fork]);
        printf("哲学家 %d 放下左筷子 %d。\n", id + 1, left_fork);
        // 离开尝试拿筷子的区域，释放 room 信号量名额
        sem_post(&room);

        printf("哲学家 %d 进餐结束。\n", id + 1);
    }

    return NULL;
}
// 哲学家进餐问题模拟
static void run_dining_philosophers()
{
    pthread_t philosophers[PHILOSOPHER_COUNT]; // 哲学家线程数组
    int philosopher_ids[PHILOSOPHER_COUNT]; // 哲学家编号数组
    int i;

    printf("\n========== 哲学家进餐问题 ==========\n");
    printf("哲学家数量：%d\n", PHILOSOPHER_COUNT);
    printf("每位哲学家进餐次数：%d\n", PHILOSOPHER_EAT_TIMES);
    printf("避免死锁策略：room 信号量限制最多 4 位哲学家同时尝试拿筷子。\n");
    printf("----------------------------------\n");

    // 初始化 room 信号量，最多允许 PHILOSOPHER_COUNT - 1 个哲学家同时尝试拿筷子
    sem_init(&room, 0, PHILOSOPHER_COUNT - 1);
    // 初始化每一把筷子的互斥锁
    for (i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        pthread_mutex_init(&forks[i], NULL);
    }
    // 创建哲学家线程
    for (i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        philosopher_ids[i] = i;
        pthread_create(&philosophers[i], NULL,
            philosopher_thread, &philosopher_ids[i]);
    }

    for (i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        pthread_join(philosophers[i], NULL);
    }

    for (i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        pthread_mutex_destroy(&forks[i]);
    }

    sem_destroy(&room);

    printf("----------------------------------\n");
    printf("哲学家进餐问题模拟结束。\n");
}

// 进程同步与并发控制模块菜单
void sync_menu()
{
    int choice;

    while (1)
    {
        printf("\n");
        printf("-------- 进程同步与并发控制模块 --------\n");
        printf("  1. 生产者—消费者问题\n");
        printf("  2. 读者—写者问题\n");
        printf("  3. 哲学家进餐问题\n");
        printf("  0. 返回主菜单\n");
        printf("----------------------------------------\n");
        printf("请输入你的选择：");

        if (scanf("%d", &choice) != 1)
        {
            printf("输入无效，请重新输入。\n");
            sync_clear_input_buffer();
            continue;
        }

        switch (choice)
        {
        case 1:
            run_producer_consumer();
            break;
        case 2:
            run_readers_writers();
            break;
        case 3:
            run_dining_philosophers();
            break;
        case 0:
            return;
        default:
            printf("选项不存在，请重新输入。\n");
            break;
        }
    }
}