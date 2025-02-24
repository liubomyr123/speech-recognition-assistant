# Define the compiler to be used
CC = gcc

# Include directories
INCLUDE_DIRS = -I/usr/local/include -I/usr/local/include/pocketsphinx

# Compiler flags
CFLAGS = -Wall -Wextra ${INCLUDE_DIRS}

# Linker flags
LDFLAGS = -L/usr/local/lib -lpocketsphinx

# Name of the output executable
TARGET = live

# Source files
SRCS = live.c

# Object files
OBJS = $(SRCS:.c=.o)

ifeq ($(an4), 1)
    CFLAGS += -Dan4
endif

ifeq ($(MODEL_RU), 1)
    CFLAGS += -DMODEL_RU
endif

ifeq ($(MODEL_UA), 1)
    CFLAGS += -DMODEL_UA
endif

ifeq ($(MODEL_DE), 1)
    CFLAGS += -DMODEL_DE
endif

# Rule to build the executable
$(TARGET): clean $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(TARGET) $(OBJS)

# gcc -o live live.c -I/usr/local/include -I/usr/local/include/pocketsphinx -L/usr/local/lib -lpocketsphinx
