#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

#define DISK_BLOCK_COUNT 64 // 模拟磁盘磁盘块数
#define BLOCK_SIZE 64 // 磁盘块大小
#define MAX_FILE_COUNT 16 // 目录表最多支持文件个数
#define MAX_FILE_NAME 20 // 文件名最大长度
#define MAX_FILE_CONTENT 1024 // 单次输入的文件内容最多支持的字符数

// 目录项结构体
typedef struct
{
    char file_name[MAX_FILE_NAME]; // 文件名
    int file_size; // 文件大小，单位为字节
    int block_count; // 文件占用的磁盘块数量
    int blocks[DISK_BLOCK_COUNT]; // 文件占用的磁盘块编号
    int used; // 目录项是否被使用
} DirectoryEntry;

// 模拟磁盘
// disk[i] 表示第 i 个磁盘块；每个磁盘块大小为 BLOCK_SIZE 字节
static char disk[DISK_BLOCK_COUNT][BLOCK_SIZE];

// bitmap 位示图
// bitmap[i] = 0 表示第 i 个磁盘块空闲； 1 表示第 i 个磁盘块已被占用
static int bitmap[DISK_BLOCK_COUNT];

// 简单目录表
static DirectoryEntry directory[MAX_FILE_COUNT];

//* 文件系统是否已经初始化
static int filesystem_initialized = 0;

// 清空输入缓冲区
static void filesystem_clear_input_buffer()
{
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

// 初始化文件系统
static void init_filesystem()
{
    int i;
    int j;

    for (i = 0; i < DISK_BLOCK_COUNT; i++)
    {
        bitmap[i] = 0;

        for (j = 0; j < BLOCK_SIZE; j++)
        {
            disk[i][j] = '\0';
        }
    }

    for (i = 0; i < MAX_FILE_COUNT; i++)
    {
        directory[i].used = 0;
        directory[i].file_size = 0;
        directory[i].block_count = 0;
        directory[i].file_name[0] = '\0';

        for (j = 0; j < DISK_BLOCK_COUNT; j++)
        {
            directory[i].blocks[j] = -1;
        }
    }

    filesystem_initialized = 1;

    printf("模拟文件系统初始化完成。\n");
    printf("磁盘块数量：%d，每块大小：%d 字节。\n",
        DISK_BLOCK_COUNT, BLOCK_SIZE);
    printf("最多支持文件数量：%d。\n", MAX_FILE_COUNT);
}

// 确保文件系统已初始化
static void ensure_filesystem_initialized()
{
    if (!filesystem_initialized)
    {
        init_filesystem();
    }
}

// 查找文件，找到返回目录项下标，否则返回 -1
static int find_file(const char* file_name)
{
    int i;

    for (i = 0; i < MAX_FILE_COUNT; i++)
    {
        if (directory[i].used &&
            strcmp(directory[i].file_name, file_name) == 0)
        {
            return i;
        }
    }

    return -1;
}

// 查找空闲目录项
static int find_free_directory_entry()
{
    int i;

    for (i = 0; i < MAX_FILE_COUNT; i++)
    {
        if (!directory[i].used)
        {
            return i;
        }
    }

    return -1;
}

// 统计空闲磁盘块数量
static int count_free_blocks()
{
    int i;
    int count = 0;

    for (i = 0; i < DISK_BLOCK_COUNT; i++)
    {
        if (bitmap[i] == 0)
        {
            count++;
        }
    }

    return count;
}

// 显示空闲空间 bitmap
static void show_bitmap()
{
    int i;

    ensure_filesystem_initialized();

    printf("\n---------------- 空闲空间 bitmap ----------------\n");
    printf("说明：0 表示空闲，1 表示已占用。\n\n");

    for (i = 0; i < DISK_BLOCK_COUNT; i++)
    {
        printf("%d", bitmap[i]);

        if ((i + 1) % 8 == 0)
        {
            printf(" ");
        }

        if ((i + 1) % 32 == 0)
        {
            printf("\n");
        }
    }

    printf("\n空闲磁盘块数量：%d / %d\n",
        count_free_blocks(), DISK_BLOCK_COUNT);
    printf("------------------------------------------------\n");
}

// 显示目录表
static void show_directory()
{
    int i;
    int j;
    int has_file = 0;

    ensure_filesystem_initialized();

    printf("\n---------------- 文件目录表 ----------------\n");
    printf("编号\t文件名\t\t大小\t占用块数\t磁盘块编号\n");
    printf("--------------------------------------------\n");

    for (i = 0; i < MAX_FILE_COUNT; i++)
    {
        if (directory[i].used)
        {
            has_file = 1;

            printf("%d\t%-12s\t%d\t%d\t\t",
                i + 1,
                directory[i].file_name,
                directory[i].file_size,
                directory[i].block_count);

            for (j = 0; j < directory[i].block_count; j++)
            {
                printf("%d ", directory[i].blocks[j]);
            }

            printf("\n");
        }
    }

    if (!has_file)
    {
        printf("当前目录为空。\n");
    }

    printf("--------------------------------------------\n");
}

// 为文件分配磁盘块
// needed_blocks：需要分配的磁盘块数量
// allocated_blocks：保存分配到的磁盘块编号
static int allocate_blocks(int needed_blocks, int allocated_blocks[])
{
    int i;
    int count = 0;

    if (count_free_blocks() < needed_blocks)
    {
        return 0;
    }

    // 采用简单的空闲块扫描方式：从低编号磁盘块开始，找到空闲块就分配。
    for (i = 0; i < DISK_BLOCK_COUNT && count < needed_blocks; i++)
    {
        if (bitmap[i] == 0)
        {
            bitmap[i] = 1;
            allocated_blocks[count] = i;
            count++;
        }
    }
    // 成功返回 1，失败返回 0
    return 1;
}

// 释放文件占用的磁盘块
static void free_blocks(int blocks[], int block_count)
{
    int i;
    int block_id;

    for (i = 0; i < block_count; i++)
    {
        block_id = blocks[i];

        if (block_id >= 0 && block_id < DISK_BLOCK_COUNT)
        {
            bitmap[block_id] = 0;
            memset(disk[block_id], 0, BLOCK_SIZE);
        }
    }
}

// 将内容写入指定磁盘块
static void write_content_to_blocks(const char* content,
    int file_size,
    int blocks[],
    int block_count)
{
    int i;
    int offset = 0;
    int copy_size;

    for (i = 0; i < block_count; i++)
    {
        memset(disk[blocks[i]], 0, BLOCK_SIZE);

        if (file_size - offset >= BLOCK_SIZE)
        {
            copy_size = BLOCK_SIZE;
        }
        else
        {
            copy_size = file_size - offset;
        }

        if (copy_size > 0)
        {
            memcpy(disk[blocks[i]], content + offset, copy_size);
        }

        offset += copy_size;
    }
}

// 从指定磁盘块读取内容
static void read_content_from_blocks(int blocks[],
    int block_count,
    int file_size,
    char output[])
{
    int i;
    int offset = 0;
    int copy_size;

    for (i = 0; i < block_count; i++)
    {
        if (file_size - offset >= BLOCK_SIZE)
        {
            copy_size = BLOCK_SIZE;
        }
        else
        {
            copy_size = file_size - offset;
        }

        if (copy_size > 0)
        {
            memcpy(output + offset, disk[blocks[i]], copy_size);
        }

        offset += copy_size;
    }

    output[file_size] = '\0';
}

// 创建文件
static void create_file()
{
    char file_name[MAX_FILE_NAME];
    int directory_index;

    ensure_filesystem_initialized();

    printf("请输入要创建的文件名：");
    scanf("%19s", file_name);

    if (find_file(file_name) != -1)
    {
        printf("创建失败：文件 %s 已存在。\n", file_name);
        return;
    }

    directory_index = find_free_directory_entry();

    if (directory_index == -1)
    {
        printf("创建失败：目录表已满，最多支持 %d 个文件。\n",
            MAX_FILE_COUNT);
        return;
    }

    directory[directory_index].used = 1;
    strcpy(directory[directory_index].file_name, file_name);
    directory[directory_index].file_size = 0;
    directory[directory_index].block_count = 0;

    printf("创建成功：文件 %s 已加入目录表。\n", file_name);
    printf("说明：当前文件为空，尚未占用磁盘块，可通过“写入文件”分配空间。\n");

    show_directory();
}

// 写入文件
// 如果文件原来已有内容，先释放原有磁盘块，再根据新内容重新分配磁盘块。
static void write_file()
{
    char file_name[MAX_FILE_NAME];
    char content[MAX_FILE_CONTENT];
    int file_index;
    int file_size;
    int needed_blocks;
    int allocated_blocks[DISK_BLOCK_COUNT];
    int i;

    ensure_filesystem_initialized();

    printf("请输入要写入的文件名：");
    scanf("%19s", file_name);

    file_index = find_file(file_name);

    if (file_index == -1)
    {
        printf("写入失败：文件 %s 不存在，请先创建文件。\n", file_name);
        return;
    }

    printf("请输入文件内容，最多 %d 个字符：\n", MAX_FILE_CONTENT - 1);
    filesystem_clear_input_buffer();

    if (fgets(content, MAX_FILE_CONTENT, stdin) == NULL)
    {
        printf("写入失败：读取输入内容失败。\n");
        return;
    }

    // 去掉 fgets 读入的换行符
    content[strcspn(content, "\n")] = '\0';

    file_size = strlen(content);

    if (file_size == 0)
    {
        // 写入空内容，相当于清空文件
        free_blocks(directory[file_index].blocks,
            directory[file_index].block_count);

        directory[file_index].file_size = 0;
        directory[file_index].block_count = 0;

        for (i = 0; i < DISK_BLOCK_COUNT; i++)
        {
            directory[file_index].blocks[i] = -1;
        }

        printf("写入成功：文件 %s 已被清空。\n", file_name);
        show_directory();
        show_bitmap();
        return;
    }

    needed_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (needed_blocks > DISK_BLOCK_COUNT)
    {
        printf("写入失败：文件内容过大。\n");
        return;
    }

    // 先释放旧内容占用的磁盘块
    free_blocks(directory[file_index].blocks,
        directory[file_index].block_count);

    for (i = 0; i < DISK_BLOCK_COUNT; i++)
    {
        directory[file_index].blocks[i] = -1;
    }

    // 再为新内容分配磁盘块
    if (!allocate_blocks(needed_blocks, allocated_blocks))
    {
        printf("写入失败：空闲磁盘块不足，需要 %d 块，当前空闲 %d 块。\n",
            needed_blocks,
            count_free_blocks());

        directory[file_index].file_size = 0;
        directory[file_index].block_count = 0;
        return;
    }

    for (i = 0; i < needed_blocks; i++)
    {
        directory[file_index].blocks[i] = allocated_blocks[i];
    }

    directory[file_index].file_size = file_size;
    directory[file_index].block_count = needed_blocks;

    write_content_to_blocks(content,
        file_size,
        directory[file_index].blocks,
        directory[file_index].block_count);

    printf("写入成功：文件 %s 写入 %d 字节，占用 %d 个磁盘块。\n",
        file_name,
        file_size,
        needed_blocks);

    printf("分配的磁盘块编号：");
    for (i = 0; i < needed_blocks; i++)
    {
        printf("%d ", directory[file_index].blocks[i]);
    }
    printf("\n");

    show_directory();
    show_bitmap();
}

// 读取文件
static void read_file()
{
    char file_name[MAX_FILE_NAME];
    char output[MAX_FILE_CONTENT + 1];
    int file_index;

    ensure_filesystem_initialized();

    printf("请输入要读取的文件名：");
    scanf("%19s", file_name);

    file_index = find_file(file_name);

    if (file_index == -1)
    {
        printf("读取失败：文件 %s 不存在。\n", file_name);
        return;
    }

    if (directory[file_index].file_size == 0)
    {
        printf("文件 %s 当前为空。\n", file_name);
        return;
    }

    read_content_from_blocks(directory[file_index].blocks,
        directory[file_index].block_count,
        directory[file_index].file_size,
        output);

    printf("\n读取成功：文件 %s 的内容如下：\n", file_name);
    printf("--------------------------------------------\n");
    printf("%s\n", output);
    printf("--------------------------------------------\n");

    printf("文件大小：%d 字节，占用磁盘块数：%d。\n",
        directory[file_index].file_size,
        directory[file_index].block_count);
}

// 删除文件
static void delete_file()
{
    char file_name[MAX_FILE_NAME];
    int file_index;
    int i;

    ensure_filesystem_initialized();

    printf("请输入要删除的文件名：");
    scanf("%19s", file_name);

    file_index = find_file(file_name);

    if (file_index == -1)
    {
        printf("删除失败：文件 %s 不存在。\n", file_name);
        return;
    }

    free_blocks(directory[file_index].blocks,
        directory[file_index].block_count);

    directory[file_index].used = 0;
    directory[file_index].file_size = 0;
    directory[file_index].block_count = 0;
    directory[file_index].file_name[0] = '\0';

    for (i = 0; i < DISK_BLOCK_COUNT; i++)
    {
        directory[file_index].blocks[i] = -1;
    }

    printf("删除成功：文件 %s 已删除，其占用的磁盘块已释放。\n", file_name);

    show_directory();
    show_bitmap();
}

// 显示文件系统说明
static void show_filesystem_info()
{
    ensure_filesystem_initialized();

    printf("\n---------------- 文件系统参数 ----------------\n");
    printf("模拟磁盘块数量：%d\n", DISK_BLOCK_COUNT);
    printf("每个磁盘块大小：%d 字节\n", BLOCK_SIZE);
    printf("磁盘总容量：%d 字节\n", DISK_BLOCK_COUNT * BLOCK_SIZE);
    printf("最多支持文件数量：%d\n", MAX_FILE_COUNT);
    printf("当前空闲磁盘块数量：%d\n", count_free_blocks());
    printf("空闲空间管理方式：bitmap 位示图\n");
    printf("文件组织方式：目录表 + 非连续磁盘块分配\n");
    printf("----------------------------------------------\n");
}

// 文件系统模块菜单
void filesystem_menu()
{
    int choice;

    ensure_filesystem_initialized();

    while (1)
    {
        printf("\n");
        printf("-------------- 文件系统模块 --------------\n");
        printf("  1. 创建文件\n");
        printf("  2. 写入文件\n");
        printf("  3. 读取文件\n");
        printf("  4. 删除文件\n");
        printf("  5. 显示目录\n");
        printf("  6. 显示空闲空间 bitmap\n");
        printf("  7. 显示文件系统参数\n");
        printf("  0. 返回主菜单\n");
        printf("------------------------------------------\n");
        printf("请输入你的选择：");

        if (scanf("%d", &choice) != 1)
        {
            printf("输入无效，请重新输入。\n");
            filesystem_clear_input_buffer();
            continue;
        }

        switch (choice)
        {
        case 1:
            create_file();
            break;
        case 2:
            write_file();
            break;
        case 3:
            read_file();
            break;
        case 4:
            delete_file();
            break;
        case 5:
            show_directory();
            break;
        case 6:
            show_bitmap();
            break;
        case 7:
            show_filesystem_info();
            break;
        case 0:
            return;
        default:
            printf("选项不存在，请重新输入。\n");
            break;
        }
    }
}