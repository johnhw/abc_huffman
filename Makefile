# Makefile for compiling huffman.c and huffman_tunes.c

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Source files
SRCS = huffman.c huffman_tunes.c abc_tests.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = huffman.h huffman_tunes.h abc_tests.h

# Target executable
TARGET = huffman_app

# Phony targets
.PHONY: all clean

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Rule to build object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)