# Makefile for RTOS Coach System

CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -O2
LDFLAGS = -pthread -lrt -lm

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/coach_rtos

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Default target
all: directories $(TARGET)

# Create directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Link
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build artifacts"

# Run
run: all
	./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all

# Install (optional)
install: all
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/"

.PHONY: all clean run debug install directories