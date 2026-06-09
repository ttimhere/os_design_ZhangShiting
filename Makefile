# 缂栬瘧鍣?CC = gcc

# 缂栬瘧閫夐」
CFLAGS = -Wall -g -Iinclude

# pthread 鐢ㄤ簬杩涚▼鍚屾涓庡苟鍙戞帶鍒舵ā鍧?LDFLAGS = -pthread

# 鍙墽琛屾枃浠跺悕绉?TARGET = os_zst

# 婧愭枃浠?SRCS = src/main.c \
       src/scheduler.c \
       src/memory.c \
       src/sync.c \
       src/filesystem.c

# 鐩爣鏂囦欢
OBJS = $(SRCS:.c=.o)

# 榛樿鐩爣
all: $(TARGET)

# 閾炬帴鐢熸垚鍙墽琛屾枃浠?$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 缂栬瘧 .c 鏂囦欢涓?.o 鏂囦欢
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 娓呯悊缂栬瘧缁撴灉
clean:
	rm -f $(OBJS) $(TARGET)

# 閲嶆柊缂栬瘧
rebuild: clean all