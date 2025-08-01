#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_ALLOCATIONS 100000
#define MAX_BACKTRACE 16
#define HASH_SIZE 10007

typedef struct allocation {
    void *ptr;
    size_t size;
    void *backtrace[MAX_BACKTRACE];
    int backtrace_size;
    time_t timestamp;
    struct allocation *next;
} allocation_t;

typedef struct {
    allocation_t *table[HASH_SIZE];
    pthread_mutex_t mutex;
    size_t total_allocated;
    size_t total_freed;
    size_t peak_usage;
    size_t current_usage;
    int allocation_count;
    int free_count;
} memory_tracker_t;

static memory_tracker_t tracker = {0};
static int initialized = 0;
static int tracking_enabled = 1;

// Function pointers for original malloc/free
static void* (*real_malloc)(size_t size) = NULL;
static void (*real_free)(void *ptr) = NULL;
static void* (*real_calloc)(size_t nmemb, size_t size) = NULL;
static void* (*real_realloc)(void *ptr, size_t size) = NULL;

// Hash function for allocation tracking
static unsigned int hash_ptr(void *ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (addr >> 3) % HASH_SIZE;
}

// Initialize the tracker
static void init_tracker() {
    if (initialized) return;
    
    // Get real function pointers
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    real_free = dlsym(RTLD_NEXT, "free");
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    
    if (!real_malloc || !real_free || !real_calloc || !real_realloc) {
        fprintf(stderr, "Memory Tracker: Failed to get real function pointers\n");
        return;
    }
    
    pthread_mutex_init(&tracker.mutex, NULL);
    initialized = 1;
    
    // Register signal handler for leak report
    signal(SIGTERM, NULL);
    signal(SIGINT, NULL);
    
    // Check if we should enable tracking
    char *env = getenv("MEMTRACK_ENABLE");
    if (env && strcmp(env, "0") == 0) {
        tracking_enabled = 0;
    }
    
    fprintf(stderr, "Memory Tracker: Initialized (PID: %d)\n", getpid());
}

// Add allocation to tracker
static void track_allocation(void *ptr, size_t size) {
    if (!tracking_enabled || !initialized) return;
    
    allocation_t *alloc = real_malloc(sizeof(allocation_t));
    if (!alloc) return;
    
    alloc->ptr = ptr;
    alloc->size = size;
    alloc->timestamp = time(NULL);
    alloc->backtrace_size = backtrace(alloc->backtrace, MAX_BACKTRACE);
    
    pthread_mutex_lock(&tracker.mutex);
    
    unsigned int index = hash_ptr(ptr);
    alloc->next = tracker.table[index];
    tracker.table[index] = alloc;
    
    tracker.total_allocated += size;
    tracker.current_usage += size;
    tracker.allocation_count++;
    
    if (tracker.current_usage > tracker.peak_usage) {
        tracker.peak_usage = tracker.current_usage;
    }
    
    pthread_mutex_unlock(&tracker.mutex);
}

// Remove allocation from tracker
static void untrack_allocation(void *ptr) {
    if (!tracking_enabled || !initialized || !ptr) return;
    
    pthread_mutex_lock(&tracker.mutex);
    
    unsigned int index = hash_ptr(ptr);
    allocation_t **current = &tracker.table[index];
    
    while (*current) {
        if ((*current)->ptr == ptr) {
            allocation_t *to_remove = *current;
            *current = (*current)->next;
            
            tracker.total_freed += to_remove->size;
            tracker.current_usage -= to_remove->size;
            tracker.free_count++;
            
            real_free(to_remove);
            pthread_mutex_unlock(&tracker.mutex);
            return;
        }
        current = &(*current)->next;
    }
    
    pthread_mutex_unlock(&tracker.mutex);
    fprintf(stderr, "Memory Tracker: WARNING - Free of untracked pointer %p\n", ptr);
}

// Print leak report
void print_leak_report() {
    if (!initialized) return;
    
    pthread_mutex_lock(&tracker.mutex);
    
    fprintf(stderr, "\n=== MEMORY LEAK REPORT ===\n");
    fprintf(stderr, "Total allocated: %zu bytes (%d allocations)\n", 
            tracker.total_allocated, tracker.allocation_count);
    fprintf(stderr, "Total freed: %zu bytes (%d frees)\n", 
            tracker.total_freed, tracker.free_count);
    fprintf(stderr, "Current usage: %zu bytes\n", tracker.current_usage);
    fprintf(stderr, "Peak usage: %zu bytes\n", tracker.peak_usage);
    
    if (tracker.current_usage > 0) {
        fprintf(stderr, "\nLEAKED ALLOCATIONS:\n");
        
        for (int i = 0; i < HASH_SIZE; i++) {
            allocation_t *alloc = tracker.table[i];
            while (alloc) {
                fprintf(stderr, "  LEAK: %zu bytes at %p (allocated at %s", 
                        alloc->size, alloc->ptr, ctime(&alloc->timestamp));
                
                // Print backtrace
                char **symbols = backtrace_symbols(alloc->backtrace, alloc->backtrace_size);
                if (symbols) {
                    for (int j = 0; j < alloc->backtrace_size; j++) {
                        fprintf(stderr, "    %s\n", symbols[j]);
                    }
                    free(symbols);
                }
                alloc = alloc->next;
            }
        }
    } else {
        fprintf(stderr, "No memory leaks detected!\n");
    }
    
    fprintf(stderr, "=========================\n\n");
    pthread_mutex_unlock(&tracker.mutex);
}

// Intercepted malloc
void* malloc(size_t size) {
    if (!initialized) init_tracker();
    
    void *ptr = real_malloc(size);
    if (ptr) {
        track_allocation(ptr, size);
    }
    return ptr;
}

// Intercepted free
void free(void *ptr) {
    if (!initialized) init_tracker();
    
    if (ptr) {
        untrack_allocation(ptr);
        real_free(ptr);
    }
}

// Intercepted calloc
void* calloc(size_t nmemb, size_t size) {
    if (!initialized) init_tracker();
    
    void *ptr = real_calloc(nmemb, size);
    if (ptr) {
        track_allocation(ptr, nmemb * size);
    }
    return ptr;
}

// Intercepted realloc
void* realloc(void *ptr, size_t size) {
    if (!initialized) init_tracker();
    
    if (!ptr) {
        // realloc(NULL, size) is equivalent to malloc(size)
        void *new_ptr = real_malloc(size);
        if (new_ptr) {
            track_allocation(new_ptr, size);
        }
        return new_ptr;
    }
    
    if (size == 0) {
        // realloc(ptr, 0) is equivalent to free(ptr)
        untrack_allocation(ptr);
        real_free(ptr);
        return NULL;
    }
    
    void *new_ptr = real_realloc(ptr, size);
    if (new_ptr) {
        untrack_allocation(ptr);
        track_allocation(new_ptr, size);
    }
    return new_ptr;
}

// Constructor - called when library is loaded
__attribute__((constructor))
static void memory_tracker_init() {
    init_tracker();
}

// Destructor - called when library is unloaded
__attribute__((destructor))
static void memory_tracker_cleanup() {
    if (initialized) {
        print_leak_report();
    }
}