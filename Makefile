# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -g -Iinclude

# pthread 用于进程同步与并发控制模块
LDFLAGS = -pthread

# 可执行文件名称
TARGET = os_zst

# 源文件
SRCS = src/main.c \
       src/scheduler.c \
       src/memory.c \
       src/sync.c \
       src/filesystem.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 编译 .c 文件为 .o 文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译结果
clean:
	rm -f $(OBJS) $(TARGET)

# 重新编译
rebuild: clean all