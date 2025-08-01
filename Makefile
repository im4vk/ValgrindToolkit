CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -g -fPIC -O2
CXXFLAGS = -Wall -Wextra -g -std=c++17 -O2
LDFLAGS = -ldl -lbfd -liberty

.PHONY: all clean test memory_tracker rust_profiler static_analyzer heap_tracker test_programs

all: memory_tracker rust_profiler static_analyzer heap_tracker test_programs

memory_tracker:
	@echo "Building Runtime Memory Tracker..."
	@cd memory_tracker && $(MAKE)

rust_profiler:
	@echo "Building Rust Memory Profiler..."
	@cd rust_profiler && cargo build --release

static_analyzer:
	@echo "Building Static Analysis Tool..."
	@cd static_analyzer && $(MAKE)

heap_tracker:
	@echo "Building Simple Heap Tracker..."
	@cd heap_tracker && $(MAKE)

test_programs:
	@echo "Building Test Programs..."
	@cd test_programs && $(MAKE)

test: all
	@echo "Running tests..."
	@cd test_programs && ./run_tests.sh

clean:
	@cd memory_tracker && $(MAKE) clean
	@cd rust_profiler && cargo clean
	@cd static_analyzer && $(MAKE) clean
	@cd heap_tracker && $(MAKE) clean
	@cd test_programs && $(MAKE) clean

install:
	@echo "Installing tools to /usr/local/bin..."
	@sudo cp memory_tracker/libmemtrack.so /usr/local/lib/
	@sudo cp memory_tracker/memtrack /usr/local/bin/
	@sudo cp rust_profiler/target/release/rust_profiler /usr/local/bin/
	@sudo cp static_analyzer/static_analyzer /usr/local/bin/
	@sudo cp heap_tracker/heap_tracker /usr/local/bin/
