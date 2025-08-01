/*
 * Double Free Test Program
 * 
 * This program tests detection of double-free errors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void simple_double_free() {
    printf("Testing simple double free...\n");
    
    char *ptr = malloc(100);
    strcpy(ptr, "This will be double-freed");
    printf("Allocated and used: %s\n", ptr);
    
    free(ptr);
    printf("First free completed\n");
    
    // This is the error - freeing the same pointer twice
    free(ptr);
    printf("Second free completed (this is an error!)\n");
}

void conditional_double_free(int condition) {
    printf("Testing conditional double free...\n");
    
    char *ptr = malloc(200);
    strcpy(ptr, "Conditional allocation");
    
    if (condition) {
        free(ptr);
        printf("Freed in condition branch\n");
    }
    
    // Error: might free again if condition was true
    free(ptr);
    printf("Freed after condition\n");
}

int main() {
    printf("=== Double Free Test Program ===\n");
    printf("This program will cause double-free errors\n\n");
    
    simple_double_free();
    printf("---\n");
    
    conditional_double_free(1); // This will cause double free
    
    printf("\nTest completed (if it didn't crash)\n");
    return 0;
}