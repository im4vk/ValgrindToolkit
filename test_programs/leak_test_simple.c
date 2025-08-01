/*
 * Simple Memory Leak Test Program
 * 
 * This program intentionally creates memory leaks for testing
 * our memory leak detection tools.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void simple_leak() {
    printf("Creating simple leak...\n");
    char *ptr = malloc(100);
    strcpy(ptr, "This memory will be leaked!");
    printf("Allocated 100 bytes: %s\n", ptr);
    // Intentionally not calling free(ptr)
}

void multiple_leaks() {
    printf("Creating multiple leaks...\n");
    for (int i = 0; i < 5; i++) {
        int *arr = malloc(sizeof(int) * 10);
        for (int j = 0; j < 10; j++) {
            arr[j] = i * 10 + j;
        }
        printf("Leak %d: allocated array of 10 ints\n", i + 1);
        // Intentionally not freeing arr
    }
}

void mixed_allocations() {
    printf("Mixed allocations with some frees...\n");
    
    char *ptr1 = malloc(50);
    char *ptr2 = malloc(75);
    char *ptr3 = calloc(20, sizeof(int));
    char *ptr4 = malloc(200);
    
    strcpy(ptr1, "This will be freed");
    strcpy(ptr2, "This will leak");
    strcpy(ptr4, "This will also leak");
    
    printf("Allocated 4 blocks\n");
    
    // Free some but not all
    free(ptr1);
    free(ptr3);
    
    printf("Freed 2 blocks, leaked 2 blocks\n");
    // ptr2 and ptr4 are leaked
}

char* return_allocated_memory(size_t size) {
    printf("Allocating %zu bytes in function\n", size);
    char *ptr = malloc(size);
    if (ptr) {
        snprintf(ptr, size, "Allocated %zu bytes", size);
    }
    return ptr; // Caller should free this, but won't
}

void realloc_test() {
    printf("Testing realloc scenarios...\n");
    
    char *ptr = malloc(50);
    strcpy(ptr, "Initial allocation");
    printf("Initial: %s\n", ptr);
    
    ptr = realloc(ptr, 100);
    strcat(ptr, " - expanded");
    printf("After realloc: %s\n", ptr);
    
    // This should be freed but we'll leak it
}

int main() {
    printf("=== Memory Leak Test Program ===\n");
    printf("PID: %d\n", getpid());
    printf("This program will create intentional memory leaks\n\n");
    
    // Test 1: Simple leak
    simple_leak();
    sleep(1);
    
    // Test 2: Multiple leaks
    multiple_leaks();
    sleep(1);
    
    // Test 3: Mixed allocations
    mixed_allocations();
    sleep(1);
    
    // Test 4: Function returning allocated memory
    char *leaked_ptr = return_allocated_memory(128);
    printf("Got pointer from function: %s\n", leaked_ptr);
    // Not freeing leaked_ptr
    sleep(1);
    
    // Test 5: Realloc test
    realloc_test();
    sleep(1);
    
    printf("\nTest completed. Check for memory leaks!\n");
    printf("Expected leaks:\n");
    printf("- 1 block of 100 bytes (simple_leak)\n");
    printf("- 5 blocks of 40 bytes each (multiple_leaks)\n");
    printf("- 2 blocks of 75 and 200 bytes (mixed_allocations)\n");
    printf("- 1 block of 128 bytes (return_allocated_memory)\n");
    printf("- 1 block of 100 bytes (realloc_test)\n");
    printf("Total expected leaked: ~743 bytes\n");
    
    return 0;
}