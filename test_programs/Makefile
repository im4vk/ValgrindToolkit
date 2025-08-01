CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -g -std=c99
CXXFLAGS = -Wall -Wextra -g -std=c++17

TARGETS = leak_test_simple leak_test_cpp double_free_test use_after_free_test

.PHONY: all clean test run-tests

all: $(TARGETS)

leak_test_simple: leak_test_simple.c
	$(CC) $(CFLAGS) -o $@ $<

leak_test_cpp: leak_test_cpp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

double_free_test: double_free_test.c
	$(CC) $(CFLAGS) -o $@ $<

use_after_free_test: use_after_free_test.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@echo "Building all test programs completed."
	@echo "Use 'make run-tests' to run them with our memory leak detection tools."

run-tests: all
	@echo "=== Running Memory Leak Detection Tests ==="
	@echo
	@echo "1. Testing with Memory Tracker (LD_PRELOAD):"
	@if [ -f ../memory_tracker/memtrack ]; then \
		../memory_tracker/memtrack ./leak_test_simple; \
	else \
		echo "Memory tracker not built. Run 'make' in memory_tracker/ first."; \
	fi
	@echo
	@echo "2. Testing with Static Analyzer:"
	@if [ -f ../static_analyzer/static_analyzer ]; then \
		../static_analyzer/static_analyzer .; \
	else \
		echo "Static analyzer not built. Run 'make' in static_analyzer/ first."; \
	fi
	@echo
	@echo "3. Testing with Heap Tracker:"
	@if [ -f ../heap_tracker/heap_tracker ]; then \
		../heap_tracker/heap_tracker -l heap_test.log ./leak_test_simple; \
		echo "Heap tracker log:"; \
		cat heap_test.log; \
		rm -f heap_test.log; \
	else \
		echo "Heap tracker not built. Run 'make' in heap_tracker/ first."; \
	fi
	@echo
	@echo "4. Testing C++ program:"
	@if [ -f ../memory_tracker/memtrack ]; then \
		../memory_tracker/memtrack ./leak_test_cpp; \
	else \
		echo "Memory tracker not available."; \
	fi

clean:
	rm -f $(TARGETS) *.log *.json *.md

install: all
	@echo "Test programs are not installed. Use them locally for testing."