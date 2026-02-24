# =============================================================================
# Makefile for NeonHeap: Memory Allocation Visualizer
# =============================================================================
# Usage:
#   make              — Compile the C backend
#   make run          — Compile and start the API server on port 8080
#   make debug        — Compile with debug symbols (-g) for GDB
#   make clean        — Remove compiled binaries
#   make frontend     — Install and start the React frontend
# =============================================================================

# Compiler and flags
CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11
INCLUDES = -I include
SRC      = src/main.c src/memory_manager.c src/memory_structures.c \
           src/os_memory.c src/proc_reader.c src/http_server.c
OUT_DIR  = build
TARGET   = $(OUT_DIR)/memory_visualizer
PORT     = 8080

# macOS requires libproc (linked via CoreFoundation framework)
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -framework CoreFoundation
else
    LDFLAGS =
endif

# =============================================================================
# Targets
# =============================================================================

.PHONY: all run debug clean frontend help

## Default: compile the backend
all: $(TARGET)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(TARGET): $(SRC) | $(OUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo ""
	@echo "✅ Build successful: $(TARGET)"
	@echo "   Run with: ./$(TARGET) --server $(PORT)"

## Compile and start the server
run: $(TARGET)
	./$(TARGET) --server $(PORT)

## Compile with debug symbols for GDB / LLDB
debug: $(OUT_DIR)
	$(CC) $(CFLAGS) -g -O0 $(INCLUDES) $(SRC) -o $(OUT_DIR)/memory_visualizer_debug $(LDFLAGS)
	@echo ""
	@echo "✅ Debug build: $(OUT_DIR)/memory_visualizer_debug"
	@echo "   Debug with: lldb $(OUT_DIR)/memory_visualizer_debug"
	@echo "           or: gdb  $(OUT_DIR)/memory_visualizer_debug"

## Remove build artifacts
clean:
	rm -rf $(OUT_DIR)
	@echo "🧹 Cleaned build directory"

## Install and start the React frontend
frontend:
	cd "UI for MAV" && npm install && npm run dev

## Show help
help:
	@echo "Available targets:"
	@echo "  make          — Compile the C backend"
	@echo "  make run      — Compile and start API server on port $(PORT)"
	@echo "  make debug    — Compile with -g for GDB/LLDB debugging"
	@echo "  make clean    — Remove compiled binaries"
	@echo "  make frontend — Install deps and start React dev server"
	@echo "  make help     — Show this help message"
