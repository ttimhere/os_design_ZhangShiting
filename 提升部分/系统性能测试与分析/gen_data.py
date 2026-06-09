import random

# 生成 5000 个进程用于 FCFS 测试
with open("test_fcfs.txt", "w") as f:
    f.write("5000\n")
    for _ in range(5000):
        arrival = random.randint(0, 1000)
        burst = random.randint(1, 100)
        f.write(f"{arrival} {burst}\n")

# 生成 5000 个进程用于 Priority 测试
with open("test_priority.txt", "w") as f:
    f.write("5000\n")
    for _ in range(5000):
        arrival = random.randint(0, 1000)
        burst = random.randint(1, 100)
        priority = random.randint(1, 10)
        f.write(f"{arrival} {burst} {priority}\n")

print("测试数据生成完毕！")
