# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Iinclude -Wall -g

# Linker flags
LDFLAGS = -lncurses -largp

# Source directory
SRC_DIR = src

# Object files directory
OBJ_DIR = obj

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Output binary
TARGET = a

# Default target
all: $(TARGET)

# Link the target binary
$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

# Compile the object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the object files directory
$(OBJ_DIR):
	mkdir -p $@

# Clean target
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Prevent make from doing something with a file named clean
.PHONY: all clean
