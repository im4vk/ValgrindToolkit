/*
 * C++ Memory Leak Test Program
 * 
 * This program tests C++ specific memory leaks including
 * new/delete mismatches and array allocations.
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>

class TestClass {
private:
    int* data;
    size_t size;
    
public:
    TestClass(size_t s) : size(s) {
        data = new int[size];
        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<int>(i);
        }
        std::cout << "TestClass created with " << size << " elements\n";
    }
    
    // Intentionally missing destructor - this will leak data array
    void display() {
        std::cout << "TestClass data: ";
        for (size_t i = 0; i < std::min(size, size_t(5)); i++) {
            std::cout << data[i] << " ";
        }
        std::cout << "...\n";
    }
};

void simple_new_leak() {
    std::cout << "Creating simple new leak...\n";
    int* ptr = new int(42);
    std::cout << "Allocated int with value: " << *ptr << "\n";
    // Intentionally not calling delete ptr
}

void array_leak() {
    std::cout << "Creating array leak...\n";
    double* arr = new double[100];
    for (int i = 0; i < 100; i++) {
        arr[i] = i * 1.5;
    }
    std::cout << "Allocated array of 100 doubles\n";
    // Intentionally not calling delete[] arr
}

void mixed_new_delete_errors() {
    std::cout << "Creating new/delete type mismatches...\n";
    
    // Correct usage
    int* single = new int(10);
    int* array1 = new int[50];
    delete single;
    delete[] array1;
    
    // Incorrect usage - these will cause issues
    int* single_leak = new int(20);
    int* array_leak = new int[75];
    
    // Wrong deallocation type (should be delete[], not delete)
    // This is dangerous but some tools detect it
    // delete array_leak; // Commented out as it's undefined behavior
    
    std::cout << "Created type mismatches\n";
    // single_leak and array_leak are intentionally leaked
}

void object_leaks() {
    std::cout << "Creating object leaks...\n";
    
    TestClass* obj1 = new TestClass(10);
    TestClass* obj2 = new TestClass(25);
    
    obj1->display();
    obj2->display();
    
    // Free one object but not the other
    delete obj1; // This will still leak the internal array due to missing destructor
    // obj2 is completely leaked
}

std::string* create_string_leak() {
    std::cout << "Creating string in function...\n";
    std::string* str = new std::string("This string will be leaked");
    return str;
}

void vector_no_leak() {
    std::cout << "Creating vector (should not leak)...\n";
    std::vector<int>* vec = new std::vector<int>();
    for (int i = 0; i < 1000; i++) {
        vec->push_back(i);
    }
    std::cout << "Vector size: " << vec->size() << "\n";
    delete vec; // This should properly clean up
}

void smart_pointer_demo() {
    std::cout << "Using smart pointers (should not leak)...\n";
    {
        std::unique_ptr<int[]> smart_array(new int[100]);
        for (int i = 0; i < 100; i++) {
            smart_array[i] = i * 2;
        }
        std::cout << "Smart pointer array created\n";
        // Automatically cleaned up when going out of scope
    }
    std::cout << "Smart pointer scope ended\n";
}

int main() {
    std::cout << "=== C++ Memory Leak Test Program ===\n";
    std::cout << "PID: " << getpid() << "\n";
    std::cout << "Testing C++ specific memory leaks\n\n";
    
    // Test various leak scenarios
    simple_new_leak();
    std::cout << "---\n";
    
    array_leak();
    std::cout << "---\n";
    
    mixed_new_delete_errors();
    std::cout << "---\n";
    
    object_leaks();
    std::cout << "---\n";
    
    std::string* leaked_string = create_string_leak();
    std::cout << "Got string: " << *leaked_string << "\n";
    // Not deleting leaked_string
    std::cout << "---\n";
    
    vector_no_leak();
    std::cout << "---\n";
    
    smart_pointer_demo();
    std::cout << "---\n";
    
    std::cout << "\nC++ test completed. Check for memory leaks!\n";
    std::cout << "Expected leaks:\n";
    std::cout << "- 1 int from simple_new_leak\n";
    std::cout << "- 1 array of 100 doubles from array_leak\n";
    std::cout << "- 1 int and 1 array from mixed_new_delete_errors\n";
    std::cout << "- 1 TestClass object + internal arrays from object_leaks\n";
    std::cout << "- 1 std::string from create_string_leak\n";
    
    return 0;
}