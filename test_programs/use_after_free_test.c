/*
 * Use After Free Test Program
 * 
 * This program tests detection of use-after-free errors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void simple_use_after_free() {
    printf("Testing simple use after free...\n");
    
    char *ptr = malloc(100);
    strcpy(ptr, "This will be used after free");
    printf("Before free: %s\n", ptr);
    
    free(ptr);
    printf("Memory freed\n");
    
    // This is the error - using freed memory
    printf("After free (ERROR): %s\n", ptr);
}

void write_after_free() {
    printf("Testing write after free...\n");
    
    int *arr = malloc(10 * sizeof(int));
    for (int i = 0; i < 10; i++) {
        arr[i] = i * i;
    }
    
    printf("Array filled\n");
    free(arr);
    printf("Array freed\n");
    
    // Error: writing to freed memory
    arr[0] = 999;
    printf("Wrote to freed memory (ERROR)\n");
}

void complex_use_after_free() {
    printf("Testing complex use after free scenario...\n");
    
    typedef struct {
        int id;
        char name[50];
        double value;
    } data_t;
    
    data_t *data = malloc(sizeof(data_t));
    data->id = 42;
    strcpy(data->name, "Test Data");
    data->value = 3.14159;
    
    printf("Data created: id=%d, name=%s, value=%.2f\n", 
           data->id, data->name, data->value);
    
    free(data);
    printf("Data freed\n");
    
    // Error: accessing freed structure
    printf("After free (ERROR): id=%d, name=%s\n", data->id, data->name);
}

int main() {
    printf("=== Use After Free Test Program ===\n");
    printf("This program will cause use-after-free errors\n\n");
    
    simple_use_after_free();
    printf("---\n");
    
    write_after_free();
    printf("---\n");
    
    complex_use_after_free();
    
    printf("\nTest completed (if it didn't crash)\n");
    return 0;
}