#include <stdio.h>
#include <stdlib.h>

#include "scheduler.h"
#include "memory.h"
#include "sync.h"
#include "filesystem.h"

//清空输入缓冲区 用于处理 scanf 输入异常时残留的字符
void clear_input_buffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

//显示主菜单
void show_main_menu()
{
    printf("\n");
    printf("        操作系统课程设计\n");
    printf("----------------------------------------\n");
    printf("  1. 处理机调度\n");
    printf("  2. 内存管理\n");
    printf("  3. 进程同步与并发控制\n");
    printf("  4. 文件系统\n");
    printf("  0. 退出程序\n");
    printf("----------------------------------------\n");
    printf("请输入你的选择：");
}

int main()
{
    int choice;

    while (1)
    {
        show_main_menu();

        if (scanf("%d", &choice) != 1)
        {
            printf("输入无效，请输入数字选项。\n");
            clear_input_buffer();
            continue;
        }

        switch (choice)
        {
        case 1:
            scheduler_menu();
            break;
        case 2:
            memory_menu();
            break;
        case 3:
            sync_menu();
            break;
        case 4:
            filesystem_menu();
            break;
        case 0:
            printf("程序已退出。\n");
            return 0;
        default:
            printf("选项不存在，请重新输入。\n");
            break;
        }
    }

    return 0;
}