#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <errno.h>

#define MAX_ALLOCATIONS 10000

typedef struct allocation {
    void *address;
    size_t size;
    time_t timestamp;
    int active;
} allocation_t;

typedef struct {
    allocation_t allocations[MAX_ALLOCATIONS];
    int count;
    size_t total_allocated;
    size_t total_freed;
    size_t current_usage;
    size_t peak_usage;
} heap_state_t;

static heap_state_t heap_state = {0};
static int monitoring_enabled = 1;
static FILE *log_file = NULL;

void init_heap_tracker(const char *log_filename) {
    if (log_filename) {
        log_file = fopen(log_filename, "w");
        if (!log_file) {
            fprintf(stderr, "Warning: Cannot create log file %s\n", log_filename);
            log_file = stderr;
        }
    } else {
        log_file = stderr;
    }
    
    fprintf(log_file, "=== Heap Tracker Initialized ===\n");
    fprintf(log_file, "PID: %d\n", getpid());
    fprintf(log_file, "Timestamp: %s", ctime(&(time_t){time(NULL)}));
    fflush(log_file);
}

void log_allocation(void *ptr, size_t size) {
    if (!monitoring_enabled || !ptr) return;
    
    // Find a free slot
    int slot = -1;
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        if (!heap_state.allocations[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        fprintf(log_file, "ERROR: Maximum allocations reached!\n");
        return;
    }
    
    heap_state.allocations[slot].address = ptr;
    heap_state.allocations[slot].size = size;
    heap_state.allocations[slot].timestamp = time(NULL);
    heap_state.allocations[slot].active = 1;
    
    heap_state.count++;
    heap_state.total_allocated += size;
    heap_state.current_usage += size;
    
    if (heap_state.current_usage > heap_state.peak_usage) {
        heap_state.peak_usage = heap_state.current_usage;
    }
    
    fprintf(log_file, "ALLOC: %p, size=%zu, total_usage=%zu\n", 
            ptr, size, heap_state.current_usage);
    fflush(log_file);
}

void log_deallocation(void *ptr) {
    if (!monitoring_enabled || !ptr) return;
    
    // Find the allocation
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        if (heap_state.allocations[i].active && 
            heap_state.allocations[i].address == ptr) {
            
            size_t size = heap_state.allocations[i].size;
            heap_state.allocations[i].active = 0;
            heap_state.count--;
            heap_state.total_freed += size;
            heap_state.current_usage -= size;
            
            fprintf(log_file, "FREE: %p, size=%zu, total_usage=%zu\n", 
                    ptr, size, heap_state.current_usage);
            fflush(log_file);
            return;
        }
    }
    
    fprintf(log_file, "WARNING: Free of untracked pointer %p\n", ptr);
    fflush(log_file);
}

void print_heap_summary() {
    fprintf(log_file, "\n=== HEAP TRACKER SUMMARY ===\n");
    fprintf(log_file, "Total allocated: %zu bytes\n", heap_state.total_allocated);
    fprintf(log_file, "Total freed: %zu bytes\n", heap_state.total_freed);
    fprintf(log_file, "Current usage: %zu bytes\n", heap_state.current_usage);
    fprintf(log_file, "Peak usage: %zu bytes\n", heap_state.peak_usage);
    fprintf(log_file, "Active allocations: %d\n", heap_state.count);
    
    if (heap_state.count > 0) {
        fprintf(log_file, "\nACTIVE ALLOCATIONS (POTENTIAL LEAKS):\n");
        time_t now = time(NULL);
        
        for (int i = 0; i < MAX_ALLOCATIONS; i++) {
            if (heap_state.allocations[i].active) {
                allocation_t *alloc = &heap_state.allocations[i];
                double age = difftime(now, alloc->timestamp);
                fprintf(log_file, "  %p: %zu bytes (age: %.0f seconds)\n", 
                        alloc->address, alloc->size, age);
            }
        }
    } else {
        fprintf(log_file, "No memory leaks detected!\n");
    }
    
    fprintf(log_file, "===========================\n");
    fflush(log_file);
}

void signal_handler(int sig) {
    monitoring_enabled = 0;
    print_heap_summary();
    if (log_file && log_file != stderr) {
        fclose(log_file);
    }
    exit(0);
}

// Simple wrapper functions for basic heap tracking
void* tracked_malloc(size_t size) {
    void *ptr = malloc(size);
    log_allocation(ptr, size);
    return ptr;
}

void* tracked_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    log_allocation(ptr, nmemb * size);
    return ptr;
}

void* tracked_realloc(void *ptr, size_t size) {
    if (ptr) {
        log_deallocation(ptr);
    }
    void *new_ptr = realloc(ptr, size);
    if (new_ptr && size > 0) {
        log_allocation(new_ptr, size);
    }
    return new_ptr;
}

void tracked_free(void *ptr) {
    log_deallocation(ptr);
    free(ptr);
}

// Command-line tool functionality
void print_usage(const char *prog_name) {
    printf("Simple Heap Tracker\n");
    printf("Usage: %s [OPTIONS] COMMAND [ARGS...]\n\n", prog_name);
    printf("Options:\n");
    printf("  -l, --log FILE    Write log to FILE (default: stderr)\n");
    printf("  -s, --summary     Show summary at exit\n");
    printf("  -h, --help        Show this help\n\n");
    printf("Examples:\n");
    printf("  %s ./my_program\n", prog_name);
    printf("  %s -l heap.log ./my_program arg1 arg2\n", prog_name);
    printf("  %s -s ./my_program\n", prog_name);
}

int main(int argc, char *argv[]) {
    char *log_filename = NULL;
    int show_summary = 1;
    int arg_index = 1;
    
    // Parse command line options
    while (arg_index < argc && argv[arg_index][0] == '-') {
        if (strcmp(argv[arg_index], "-l") == 0 || strcmp(argv[arg_index], "--log") == 0) {
            if (arg_index + 1 >= argc) {
                fprintf(stderr, "Error: -l requires a filename\n");
                return 1;
            }
            log_filename = argv[++arg_index];
        } else if (strcmp(argv[arg_index], "-s") == 0 || strcmp(argv[arg_index], "--summary") == 0) {
            show_summary = 1;
        } else if (strcmp(argv[arg_index], "-h") == 0 || strcmp(argv[arg_index], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[arg_index]);
            print_usage(argv[0]);
            return 1;
        }
        arg_index++;
    }
    
    if (arg_index >= argc) {
        fprintf(stderr, "Error: No command specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Initialize the tracker
    init_heap_tracker(log_filename);
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Fork and execute the target program
    pid_t child_pid = fork();
    
    if (child_pid == 0) {
        // Child process - execute the target program
        execvp(argv[arg_index], &argv[arg_index]);
        perror("execvp failed");
        exit(1);
    } else if (child_pid > 0) {
        // Parent process - monitor the child
        int status;
        fprintf(log_file, "Monitoring process PID: %d\n", child_pid);
        
        // Wait for child to complete
        waitpid(child_pid, &status, 0);
        
        if (WIFEXITED(status)) {
            fprintf(log_file, "Process exited with status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(log_file, "Process terminated by signal: %d\n", WTERMSIG(status));
        }
        
        if (show_summary) {
            print_heap_summary();
        }
        
        if (log_file && log_file != stderr) {
            fclose(log_file);
        }
        
        return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    } else {
        perror("fork failed");
        return 1;
    }
}