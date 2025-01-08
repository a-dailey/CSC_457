# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

# Target executable
TARGET = pipeit

# Source file
SRC = pipeit.c

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up build files
clean:
	rm -f $(TARGET)