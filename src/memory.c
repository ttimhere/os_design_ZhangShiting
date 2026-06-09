#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

#define MAX_PAGE_SEQUENCE 100 // 页面访问序列的最大长度
#define MAX_FRAMES 20 // 物理块最大数量
#define MAX_PARTITIONS 50 // 最大动态分区数量
#define MAX_PROCESS_NAME 20 // 进程名称最大长度

// 动态分区结构体
typedef struct
{
    int start; // 分区起始地址
    int size; // 分区大小
    int free; // 是否空闲：1 表示空闲，0 表示已分配
    char process_name[MAX_PROCESS_NAME]; // 占用该分区的进程名
} Partition;

// 清空输入缓冲区
static void memory_clear_input_buffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

// 判断页面是否在物理块中：如果找到，返回对应下标；否则返回 -1
static int find_page_in_frames(int frames[], int frame_count, int page)
{
    int i;
    for (i = 0; i < frame_count; i++)
    {
        if (frames[i] == page)
        {
            return i;
        }
    }

    return -1;
}

// 打印当前物理块状态
static void print_frames(int frames[], int frame_count)
{
    int i;

    printf("当前物理块状态：");

    for (i = 0; i < frame_count; i++)
    {
        if (frames[i] == -1)
        {
            printf("[空] ");
        }
        else
        {
            printf("[%d] ", frames[i]);
        }
    }

    printf("\n");
}

// 输入页面访问序列
static int input_page_sequence(int pages[])
{
    int n;
    int i;

    printf("请输入页面访问序列长度，最多 %d 个：", MAX_PAGE_SEQUENCE);

    if (scanf("%d", &n) != 1)
    {
        printf("输入无效。\n");
        memory_clear_input_buffer();
        return 0;
    }

    if (n <= 0 || n > MAX_PAGE_SEQUENCE)
    {
        printf("页面访问序列长度不合法。\n");
        return 0;
    }

    printf("请依次输入页面号：\n");

    for (i = 0; i < n; i++)
    {
        printf("第 %d 个页面号：", i + 1);

        if (scanf("%d", &pages[i]) != 1 || pages[i] < 0)
        {
            printf("页面号输入不合法。\n");
            memory_clear_input_buffer();
            return 0;
        }
    }

    return n;
}

// 输入物理块数量
static int input_frame_count()
{
    int frame_count;

    printf("请输入物理块数量，最多 %d 个：", MAX_FRAMES);

    if (scanf("%d", &frame_count) != 1)
    {
        printf("输入无效。\n");
        memory_clear_input_buffer();
        return 0;
    }

    if (frame_count <= 0 || frame_count > MAX_FRAMES)
    {
        printf("物理块数量不合法。\n");
        return 0;
    }

    return frame_count;
}

// FIFO 页面置换算法
static void fifo_page_replacement()
{
    int pages[MAX_PAGE_SEQUENCE]; // 页面访问序列数组，用于保存用户输入的一系列页面号
    int frames[MAX_FRAMES]; // 物理块数组，用于保存当前已经装入内存的页面号
    int page_count; // 页面访问序列长度
    int frame_count; // 物理块数量
    int page_faults = 0; // 缺页次数
    int replace_index = 0; // FIFO 替换指针
    int i; 
    int j; 
    int page; // 当前正在访问的页面号
    int hit_index; // 页面命中的物理块下标

    page_count = input_page_sequence(pages);
    if (page_count == 0)
    {
        return;
    }

    frame_count = input_frame_count();
    if (frame_count == 0)
    {
        return;
    }

    // 初始化物理块为空
    for (i = 0; i < frame_count; i++)
    {
        frames[i] = -1;
    }

    printf("\n FIFO 页面置换过程 \n");

    // 按照用户输入的页面访问序列逐个访问页面
    for (i = 0; i < page_count; i++)
    {
        page = pages[i]; // 取出当前要访问的页面号
        printf("\n访问页面：%d\n", page);
        // 查找当前页面是否已经在物理块中
        hit_index = find_page_in_frames(frames, frame_count, page);

        if (hit_index != -1)
        {
            printf("结果：命中，页面 %d 已在物理块中。\n", page);
        }
        else
        {
            // 页面不在物理块中，发生缺页
            printf("结果：缺页。\n");
            page_faults++;

            // 如果还有空块，放入空块，否则替换最早进入的页面
            frames[replace_index] = page;
            printf("操作：将页面 %d 装入物理块 %d。\n", page, replace_index + 1);

            // 替换指针后移，超过最后一个物理块后回到第 0 个物理块
            replace_index = (replace_index + 1) % frame_count;
        }

        print_frames(frames, frame_count);
    }

    printf("\n FIFO 页面置换统计 \n");
    printf("页面访问次数：%d\n", page_count);
    printf("缺页次数：%d\n", page_faults);
    printf("缺页率：%.2f%%\n", (double)page_faults / page_count * 100.0);

    // 避免未使用变量警告
    (void)j;
}

// 查找 LRU 算法中最久未使用的页面下标
static int find_lru_index(int last_used[], int frame_count)
{
    int i;
    int min_time = last_used[0]; // 当前找到的最小最近使用时间
    int index = 0; // 当前最久未使用页面所在的物理块下标
    // 从第 1 个物理块开始比较最近使用时间
    for (i = 1; i < frame_count; i++)
    {
        // 如果某个物理块的最近使用时间更早，则更新最久未使用位置
        if (last_used[i] < min_time)
        {
            min_time = last_used[i];
            index = i;
        }
    }

    return index;
}

// LRU 页面置换算法
static void lru_page_replacement()
{
    int pages[MAX_PAGE_SEQUENCE]; // 页面访问序列数组
    int frames[MAX_FRAMES]; // 物理块数组
    int last_used[MAX_FRAMES]; // 最近使用时间数组
    int page_count; // 页面访问序列长度
    int frame_count; // 物理块数量
    int page_faults = 0; // 缺页次数
    int current_time = 0; // 当前逻辑时间，每访问一个页面加 1，用于判断页面最近使用情况
    int i; 
    int page; // 当前正在访问的页面号
    int hit_index; // 命中的物理块下标
    int empty_index; // 空闲物理块下标
    int replace_index; // 需要被替换的物理块下标

    page_count = input_page_sequence(pages);
    if (page_count == 0)
    {
        return;
    }

    frame_count = input_frame_count();
    if (frame_count == 0)
    {
        return;
    }

    // 初始化物理块和最近使用时间
    for (i = 0; i < frame_count; i++)
    {
        frames[i] = -1; // -1 表示当前物理块为空
        last_used[i] = -1; // -1 表示该物理块还没有页面被访问过
    }

    printf("\n LRU 页面置换过程 \n");

    for (i = 0; i < page_count; i++)
    {
        page = pages[i];
        current_time++;

        printf("\n访问页面：%d\n", page);
        // 判断当前页面是否已经在物理块中
        hit_index = find_page_in_frames(frames, frame_count, page);

        if (hit_index != -1)
        {
            printf("结果：命中，页面 %d 已在物理块中。\n", page);
            last_used[hit_index] = current_time;
        }
        else
        {
            // 页面未命中，发生缺页
            printf("结果：缺页。\n");
            page_faults++;
            // 先查找是否还有空闲物理块
            empty_index = find_page_in_frames(frames, frame_count, -1);

            if (empty_index != -1)
            {
                // 如果还有空闲物理块，则直接将页面装入空闲物理块
                frames[empty_index] = page;
                last_used[empty_index] = current_time;
                printf("操作：将页面 %d 装入空闲物理块 %d。\n", page, empty_index + 1);
            }
            else
            {
                // 如果没有空闲物理块，则根据 LRU 规则找到最久未使用的页面
                replace_index = find_lru_index(last_used, frame_count);
                printf("操作：页面 %d 最久未使用，将其替换为页面 %d。\n",
                    frames[replace_index],
                    page);

                frames[replace_index] = page;
                last_used[replace_index] = current_time;
            }
        }

        print_frames(frames, frame_count);
    }

    printf("\n LRU 页面置换统计 \n");
    printf("页面访问次数：%d\n", page_count);
    printf("缺页次数：%d\n", page_faults);
    printf("缺页率：%.2f%%\n", (double)page_faults / page_count * 100.0);

}

// 初始化动态分区
static void init_partitions(Partition partitions[], int* partition_count, int memory_size)
{
    // 初始时只有一个完整的大空闲分区
    *partition_count = 1;

    partitions[0].start = 0; // 初始空闲分区从地址 0 开始
    partitions[0].size = memory_size; // 初始空闲分区大小等于用户输入的总内存大小
    partitions[0].free = 1; // 设置为空闲状态
    strcpy(partitions[0].process_name, "空闲");
}

// 显示动态分区表
static void show_partitions(Partition partitions[], int partition_count)
{
    int i;

    printf("\n---------------- 动态分区表 ----------------\n");
    printf("编号\t起始地址\t分区大小\t状态\t\t进程名\n");
    printf("--------------------------------------------\n");

    for (i = 0; i < partition_count; i++)
    {
        printf("%d\t%d\t\t%d\t\t%s\t\t%s\n",
            i + 1,
            partitions[i].start,
            partitions[i].size,
            partitions[i].free ? "空闲" : "已分配",
            partitions[i].process_name);
    }

    printf("--------------------------------------------\n");
}

// 首次适应 FF 分配内存
static void ff_allocate_memory(Partition partitions[], int* partition_count)
{
    char name[MAX_PROCESS_NAME]; // 用户输入的申请内存的进程名
    int size; // 用户输入的申请内存大小
    int i;
    int j;

    if (*partition_count >= MAX_PARTITIONS)
    {
        printf("分区数量已达到上限，无法继续分配。\n");
        return;
    }

    printf("请输入进程名：");
    scanf("%s", name);

    printf("请输入申请内存大小：");
    if (scanf("%d", &size) != 1 || size <= 0)
    {
        printf("申请大小不合法。\n");
        memory_clear_input_buffer();
        return;
    }

    // 检查进程名是否已存在
    for (i = 0; i < *partition_count; i++)
    {
        if (!partitions[i].free && strcmp(partitions[i].process_name, name) == 0)
        {
            printf("进程名已存在，不能重复分配。\n");
            return;
        }
    }

    // 首次适应：从前往后找第一个足够大的空闲分区
    for (i = 0; i < *partition_count; i++)
    {
        // 当前分区空闲，并且分区大小大于等于申请大小，说明可以分配
        if (partitions[i].free && partitions[i].size >= size)
        {
            if (partitions[i].size == size)
            {
                // 空闲分区大小刚好等于申请大小，直接分配整个分区
                partitions[i].free = 0;
                strcpy(partitions[i].process_name, name);

                printf("分配成功：进程 %s 获得分区，起始地址 %d，大小 %d。\n",
                    name,
                    partitions[i].start,
                    size);
            }
            else
            {
                // 空闲分区大于申请大小，需要把该分区切割成已分配分区和剩余空闲分区
                if (*partition_count >= MAX_PARTITIONS)
                {
                    printf("分区数量已达到上限，无法切割分区。\n");
                    return;
                }

                // 将 i 后面的分区整体后移一位，为新产生的空闲分区腾出位置
                for (j = *partition_count; j > i + 1; j--)
                {
                    partitions[j] = partitions[j - 1];
                }

                // 设置切割后新空闲分区的信息
                partitions[i + 1].start = partitions[i].start + size;
                partitions[i + 1].size = partitions[i].size - size;
                partitions[i + 1].free = 1;
                strcpy(partitions[i + 1].process_name, "空闲");

                // 设置当前分区为已分配分区
                partitions[i].size = size;
                partitions[i].free = 0;
                strcpy(partitions[i].process_name, name);

                (*partition_count)++;

                printf("分配成功：进程 %s 获得分区，起始地址 %d，大小 %d。\n",
                    name,
                    partitions[i].start,
                    size);
            }

            show_partitions(partitions, *partition_count);
            return;
        }
    }

    printf("分配失败：没有足够大的空闲分区。\n");
}

// 合并相邻空闲分区
static void merge_free_partitions(Partition partitions[], int* partition_count)
{
    int i;
    int j;

    i = 0;
    // 从前往后检查相邻两个分区是否都为空闲
    while (i < *partition_count - 1)
    {
        // 如果当前分区和后一个分区都空闲，则可以合并
        if (partitions[i].free && partitions[i + 1].free)
        {
            partitions[i].size += partitions[i + 1].size;

            // 删除 i + 1 分区，后面的分区前移
            for (j = i + 1; j < *partition_count - 1; j++)
            {
                partitions[j] = partitions[j + 1];
            }

            (*partition_count)--;
        }
        else
        {
            // 如果不能合并，则继续检查下一个分区
            i++;
        }
    }
}

// 回收内存
static void ff_free_memory(Partition partitions[], int* partition_count)
{
    char name[MAX_PROCESS_NAME];
    int i;

    printf("请输入要回收的进程名：");
    scanf("%s", name);

    // 在分区表中查找该进程占用的分区
    for (i = 0; i < *partition_count; i++)
    {
        // 找到已分配且进程名匹配的分区
        if (!partitions[i].free && strcmp(partitions[i].process_name, name) == 0)
        {
            // 将该分区设置为空闲
            partitions[i].free = 1;
            strcpy(partitions[i].process_name, "空闲");

            printf("回收成功：进程 %s 占用的分区已释放。\n", name);
            // 回收后尝试合并相邻空闲分区，减少外部碎片
            merge_free_partitions(partitions, partition_count);
            show_partitions(partitions, *partition_count);
            return;
        }
    }

    printf("回收失败：未找到进程 %s。\n", name);
}

// 首次适应 FF 动态分区管理菜单
static void first_fit_partition_management()
{
    Partition partitions[MAX_PARTITIONS]; // 动态分区表数组，用于保存所有分区的信息
    int partition_count = 0; // 当前分区数量
    int memory_size; // 用户输入的模拟内存总大小
    int choice; // 用户在动态分区管理菜单中的选择

    printf("请输入模拟内存总大小：");

    if (scanf("%d", &memory_size) != 1 || memory_size <= 0)
    {
        printf("内存总大小不合法。\n");
        memory_clear_input_buffer();
        return;
    }

    init_partitions(partitions, &partition_count, memory_size);

    printf("动态分区初始化完成。\n");
    show_partitions(partitions, partition_count);

    while (1)
    {
        printf("\n");
        printf("-------- 首次适应 FF 动态分区管理 --------\n");
        printf("  1. 分配内存\n");
        printf("  2. 回收内存\n");
        printf("  3. 显示分区表\n");
        printf("  0. 返回内存管理菜单\n");
        printf("------------------------------------------\n");
        printf("请输入你的选择：");

        if (scanf("%d", &choice) != 1)
        {
            printf("输入无效，请重新输入。\n");
            memory_clear_input_buffer();
            continue;
        }

        switch (choice)
        {
        case 1:
            // 分配内存
            ff_allocate_memory(partitions, &partition_count);
            break;
        case 2:
            // 回收内存
            ff_free_memory(partitions, &partition_count);
            break;
        case 3:
            // 显示当前动态分区表
            show_partitions(partitions, partition_count);
            break;
        case 0:
            // 返回内存管理菜单
            return;
        default:
            // 处理不存在的菜单选项
            printf("选项不存在，请重新输入。\n");
            break;
        }
    }
}

// 内存管理模块菜单
void memory_menu()
{
    int choice;

    while (1)
    {
        printf("\n");
        printf("-------------- 内存管理模块 --------------\n");
        printf("  1. FIFO 页面置换算法\n");
        printf("  2. LRU 页面置换算法\n");
        printf("  3. 首次适应 FF 动态分区管理\n");
        printf("  0. 返回主菜单\n");
        printf("------------------------------------------\n");
        printf("请输入你的选择：");

        if (scanf("%d", &choice) != 1)
        {
            printf("输入无效，请重新输入。\n");
            memory_clear_input_buffer();
            continue;
        }

        switch (choice)
        {
        case 1:
            fifo_page_replacement();
            break;
        case 2:
            lru_page_replacement();
            break;
        case 3:
            first_fit_partition_management();
            break;
        case 0:
            return;
        default:
            printf("选项不存在，请重新输入。\n");
            break;
        }
    }
}