# Memory Leak Detection Tools

A collection of memory leak detection programs similar to valgrind, implemented using different approaches and techniques.

## Tools Included

### 1. Runtime Memory Tracker (`memory_tracker/`)
- Uses LD_PRELOAD to intercept malloc/free calls
- Provides real-time memory leak detection
- Tracks allocation locations with stack traces

### 2. Rust Memory Profiler (`rust_profiler/`)
- Advanced memory profiling with detailed statistics
- Low overhead monitoring
- JSON output for analysis

### 3. Static Analysis Tool (`static_analyzer/`)
- Analyzes source code for potential memory leaks
- Detects unmatched malloc/free pairs
- Reports suspicious patterns

### 4. Simple Heap Tracker (`heap_tracker/`)
- Basic allocation/deallocation monitoring
- Educational implementation
- Easy to understand and modify

### 5. Test Programs (`test_programs/`)
- Programs with intentional memory leaks
- Used for testing and validating our detection tools

## Usage

Each tool has its own directory with specific build instructions and usage examples.

## Building All Tools

```bash
make all
```

## Running Tests

```bash
make test
```