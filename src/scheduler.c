#include <stdio.h>
#include <stdlib.h>
#include "scheduler.h"

#define MAX_PROCESS 20 // 最大进程数
#define RR_QUEUE_SIZE 10000 // 就绪队列数组的最大容量

// 进程结构体
typedef struct
{
    int pid;              // 进程编号
    int arrival_time;     // 到达时间
    int burst_time;       // 服务时间 / 运行时间
    int remaining_time;   // 剩余运行时间
    int finish_time;      // 完成时间
    int turnaround_time;  // 周转时间 = 完成时间 - 到达时间
    double weighted_turnaround_time; // 带权周转时间 = 周转时间 / 服务时间
} Process;

// 清空输入缓冲区
static void scheduler_clear_input_buffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

// 交换两个进程
static void swap_process(Process* a, Process* b)
{
    Process temp = *a;
    *a = *b;
    *b = temp;
}

// 按到达时间排序 如果到达时间相同 则按进程编号排序
static void sort_by_arrival_time(Process p[], int n)
{
    int i, j;

    for (i = 0; i < n - 1; i++)
    {
        for (j = 0; j < n - 1 - i; j++)
        {
            if (p[j].arrival_time > p[j + 1].arrival_time ||
                (p[j].arrival_time == p[j + 1].arrival_time &&
                    p[j].pid > p[j + 1].pid))
            {
                swap_process(&p[j], &p[j + 1]);
            }
        }
    }
}

// 输入进程信息
static int input_processes(Process p[])
{
    int n;
    int i;

    printf("请输入进程数量，最多 %d 个：", MAX_PROCESS);

    if (scanf("%d", &n) != 1)
    {
        printf("输入无效。\n");
        scheduler_clear_input_buffer();
        return 0;
    }

    if (n <= 0 || n > MAX_PROCESS)
    {
        printf("进程数量不合法。\n");
        return 0;
    }

    for (i = 0; i < n; i++)
    {
        p[i].pid = i + 1;

        printf("\n请输入进程 P%d 的到达时间：", p[i].pid);
        if (scanf("%d", &p[i].arrival_time) != 1 || p[i].arrival_time < 0)
        {
            printf("到达时间输入不合法。\n");
            scheduler_clear_input_buffer();
            return 0;
        }

        printf("请输入进程 P%d 的服务时间：", p[i].pid);
        if (scanf("%d", &p[i].burst_time) != 1 || p[i].burst_time <= 0)
        {
            printf("服务时间输入不合法。\n");
            scheduler_clear_input_buffer();
            return 0;
        }

        p[i].remaining_time = p[i].burst_time;
        p[i].finish_time = 0;
        p[i].turnaround_time = 0;
        p[i].weighted_turnaround_time = 0.0;
    }

    return n;
}

// 复制进程数组 避免一个算法修改数据后影响另一个算法
static void copy_processes(Process dest[], Process src[], int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        dest[i] = src[i];
    }
}

// 输出调度结果表
static void print_result(Process p[], int n)
{
    int i;
    double total_turnaround_time = 0.0;
    double total_weighted_turnaround_time = 0.0;

    printf("\n调度结果：\n");
    printf("进程\t到达时间\t服务时间\t完成时间\t周转时间\t带权周转时间\n");

    for (i = 0; i < n; i++)
    {
        // 周转时间 = 完成时间 - 到达时间
        p[i].turnaround_time = p[i].finish_time - p[i].arrival_time;
        // 带权周转时间 = 周转时间 / 服务时间
        p[i].weighted_turnaround_time =
            (double)p[i].turnaround_time / p[i].burst_time;

        total_turnaround_time += p[i].turnaround_time;
        total_weighted_turnaround_time += p[i].weighted_turnaround_time;

        printf("P%d\t%d\t\t%d\t\t%d\t\t%d\t\t%.2f\n",
            p[i].pid,
            p[i].arrival_time,
            p[i].burst_time,
            p[i].finish_time,
            p[i].turnaround_time,
            p[i].weighted_turnaround_time);
    }

    printf("平均周转时间：%.2f\n", total_turnaround_time / n);
    printf("平均带权周转时间：%.2f\n", total_weighted_turnaround_time / n);
}

// FCFS 先来先服务调度算法
static void fcfs_schedule()
{
    Process original[MAX_PROCESS]; // 保存用户输入的原始进程数据
    Process p[MAX_PROCESS]; // 保存当前算法使用的进程数据
    int n;
    int i;
    int current_time = 0; // 当前系统时间
    int start_time; // 当前进程开始运行的时间

    // 输入进程信息
    n = input_processes(original);
    if (n == 0)
    {
        return;
    }

    copy_processes(p, original, n);
    sort_by_arrival_time(p, n);

    printf("\nFCFS 先来先服务调度运行顺序：\n");
    // 按到达时间顺序依次执行每个进程
    for (i = 0; i < n; i++)
    {
        // 如果当前时间早于进程到达时间，说明 CPU 需要空闲等待
        if (current_time < p[i].arrival_time)
        {
            printf("[时间 %d-%d：CPU 空闲] ",
                current_time,
                p[i].arrival_time);
            // 将当前时间推进到该进程的到达时间
            current_time = p[i].arrival_time;
        }

        start_time = current_time; // 记录当前进程开始执行的时间
        current_time += p[i].burst_time;
        p[i].finish_time = current_time;

        printf("[时间 %d-%d：P%d] ",
            start_time,
            current_time,
            p[i].pid);
    }

    printf("\n");

    print_result(p, n);

}

// SJF 短作业优先调度算法
static void sjf_schedule()
{
    Process original[MAX_PROCESS]; // 保存用户输入的原始进程数据
    Process p[MAX_PROCESS]; // 保存当前算法使用的进程数据
    int n;
    int completed = 0;  // 已完成的进程数量
    int current_time = 0; // 当前系统时间
    int i;
    int selected;  // 当前被选中的进程下标
    int min_burst; // 当前最短服务时间
    int start_time; // 当前进程开始运行时间
    // 输入进程信息
    n = input_processes(original);
    if (n == 0)
    {
        return;
    }

    copy_processes(p, original, n);

    printf("\nSJF 短作业优先调度运行顺序：\n");
    // 当还有进程未完成时，继续调度
    while (completed < n)
    {
        selected = -1;
        min_burst = 1000000000;

        // 在已经到达且还未完成的进程中 选择服务时间最短者
        for (i = 0; i < n; i++)
        {
            if (p[i].remaining_time > 0 &&
                p[i].arrival_time <= current_time)
            {
                // 如果当前进程服务时间更短，则选择它
                if (p[i].burst_time < min_burst)
                {
                    min_burst = p[i].burst_time;
                    selected = i;
                }
                else if (p[i].burst_time == min_burst)
                {
                    // 服务时间相同时 先到达者优先
                    // 到达时间也相同时 进程编号小者优先
                     
                    if (p[i].arrival_time < p[selected].arrival_time ||
                        (p[i].arrival_time == p[selected].arrival_time &&
                            p[i].pid < p[selected].pid))
                    {
                        selected = i;
                    }
                }
            }
        }

        // 如果当前没有任何进程到达 则 CPU 空闲 时间跳到下一个进程的到达时间
        if (selected == -1)
        {
            int next_arrival = 1000000000;
            // 查找还未完成进程中的最早到达时间
            for (i = 0; i < n; i++)
            {
                if (p[i].remaining_time > 0 &&
                    p[i].arrival_time < next_arrival)
                {
                    next_arrival = p[i].arrival_time;
                }
            }
            // 输出 CPU 空闲区间
            printf("[时间 %d-%d：CPU 空闲] ",
                current_time,
                next_arrival);
            // 将系统时间推进到下一个进程到达时间
            current_time = next_arrival;
            continue;
        }

        start_time = current_time; // 记录被选中进程的开始运行时间
        current_time += p[selected].burst_time;
        p[selected].remaining_time = 0; // 将剩余时间置 0，表示该进程已经完成
        p[selected].finish_time = current_time; // 记录该进程完成时间
        completed++;

        printf("[时间 %d-%d：P%d] ",
            start_time,
            current_time,
            p[selected].pid);
    }
    printf("\n");
    print_result(p, n);
}

// 判断 RR 队列是否为空
static int rr_queue_empty(int front, int rear)
{
    return front == rear;
}

// RR 队列入队
static void rr_enqueue(int queue[], int* rear, int value)
{
    queue[*rear] = value;
    (*rear)++;
}

// RR 队列出队
static int rr_dequeue(int queue[], int* front)
{
    int value = queue[*front];
    (*front)++;
    return value;
}

// RR 时间片轮转调度算法
static void rr_schedule()
{
    Process original[MAX_PROCESS]; // 保存用户输入的原始进程数据
    Process p[MAX_PROCESS]; // 保存当前算法使用的进程数据
    int queue[RR_QUEUE_SIZE]; // 就绪队列，保存进程在数组中的下标
    int n;
    int time_quantum;  // 时间片大小
    int current_time = 0; // 当前系统时间
    int completed = 0; // 已完成进程数量
    int next_process = 0; // 下一个尚未进入队列的进程下标
    int front = 0; // 队头指针
    int rear = 0; // 队尾指针
    int i;
    int index; // 当前出队的进程下标
    int run_time; // 当前进程本次实际运行时间
    int start_time; // 当前进程本次开始运行时间

    n = input_processes(original);
    if (n == 0)
    {
        return;
    }

    printf("请输入时间片大小：");
    if (scanf("%d", &time_quantum) != 1 || time_quantum <= 0)
    {
        printf("时间片大小不合法。\n");
        scheduler_clear_input_buffer();
        return;
    }

    copy_processes(p, original, n);
    sort_by_arrival_time(p, n);

    // 初始化所有进程的剩余运行时间
    for (i = 0; i < n; i++)
    {
        p[i].remaining_time = p[i].burst_time;
    }

    printf("\nRR 时间片轮转调度运行顺序：\n");
    // 当还有进程没有完成时，继续进行时间片轮转
    while (completed < n)
    {
        // 如果就绪队列为空 并且还有进程未到达 则 CPU 空闲到下一个进程到达时刻
        if (rr_queue_empty(front, rear))
        {
            if (next_process < n && current_time < p[next_process].arrival_time)
            {
                printf("[时间 %d-%d：CPU 空闲] ",
                    current_time,
                    p[next_process].arrival_time);
                // 时间直接推进到下一个进程的到达时间
                current_time = p[next_process].arrival_time;
            }

            // 把当前时刻已经到达的进程加入就绪队列
            while (next_process < n &&
                p[next_process].arrival_time <= current_time)
            {
                rr_enqueue(queue, &rear, next_process);
                next_process++;
            }
        }
        // 如果队列仍然为空，则继续下一轮循环
        if (rr_queue_empty(front, rear))
        {
            continue;
        }
        // 从就绪队列中取出队首进程
        index = rr_dequeue(queue, &front);
        // 判断本次运行时间：不能超过时间片，也不能超过剩余运行时间
        if (p[index].remaining_time > time_quantum)
        {
            run_time = time_quantum;
        }
        else
        {
            run_time = p[index].remaining_time;
        }

        start_time = current_time;
        current_time += run_time;
        p[index].remaining_time -= run_time;

        printf("[时间 %d-%d：P%d] ",
            start_time,
            current_time,
            p[index].pid);

        // 在当前进程运行期间新到达的进程 先加入就绪队列
        while (next_process < n &&
            p[next_process].arrival_time <= current_time)
        {
            rr_enqueue(queue, &rear, next_process);
            next_process++;
        }

        // 当前进程没执行完 则重新排到就绪队列队尾 否则记录完成时间 
        if (p[index].remaining_time > 0)
        {
            rr_enqueue(queue, &rear, index);
        }
        else
        {
            p[index].finish_time = current_time;
            completed++;
        }
    }
    printf("\n");
    print_result(p, n);
}

// 处理机调度模块菜单
void scheduler_menu()
{
    int choice;

    while (1)
    {
        printf("\n");
        printf("------------ 处理机调度模块 ------------\n");
        printf("  1. FCFS 先来先服务调度\n");
        printf("  2. SJF 短作业优先调度\n");
        printf("  3. RR 时间片轮转调度\n");
        printf("  0. 返回主菜单\n");
        printf("----------------------------------------\n");
        printf("请输入你的选择：");

        if (scanf("%d", &choice) != 1)
        {
            printf("输入无效，请重新输入。\n");
            scheduler_clear_input_buffer();
            continue;
        }

        switch (choice)
        {
        case 1:
            fcfs_schedule();
            break;
        case 2:
            sjf_schedule();
            break;
        case 3:
            rr_schedule();
            break;
        case 0:
            return;
        default:
            printf("选项不存在，请重新输入。\n");
            break;
        }
    }
}