CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -g
TARGET = static_analyzer
SOURCE = static_analyzer.cpp

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $@ $<

test: $(TARGET)
	@echo "Creating test C files with memory leaks..."
	@mkdir -p test_files
	@echo '#include <stdlib.h>' > test_files/leak_test.c
	@echo '#include <stdio.h>' >> test_files/leak_test.c
	@echo 'int main() {' >> test_files/leak_test.c
	@echo '    char *ptr1 = malloc(100);' >> test_files/leak_test.c
	@echo '    char *ptr2 = malloc(200);' >> test_files/leak_test.c
	@echo '    free(ptr1);' >> test_files/leak_test.c
	@echo '    // ptr2 is leaked' >> test_files/leak_test.c
	@echo '    return 0;' >> test_files/leak_test.c
	@echo '}' >> test_files/leak_test.c
	@echo
	@echo '#include <iostream>' > test_files/cpp_leak_test.cpp
	@echo 'int main() {' >> test_files/cpp_leak_test.cpp
	@echo '    int *arr = new int[100];' >> test_files/cpp_leak_test.cpp
	@echo '    int *ptr = new int(42);' >> test_files/cpp_leak_test.cpp
	@echo '    delete ptr;' >> test_files/cpp_leak_test.cpp
	@echo '    // arr is leaked (should use delete[])' >> test_files/cpp_leak_test.cpp
	@echo '    return 0;' >> test_files/cpp_leak_test.cpp
	@echo '}' >> test_files/cpp_leak_test.cpp
	@echo
	@echo "Running static analysis on test files:"
	@./$(TARGET) test_files/
	@echo
	@echo "Testing with report generation:"
	@./$(TARGET) test_files/ test_report.md

clean:
	rm -f $(TARGET) test_report.md
	rm -rf test_files/

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

debug: $(SOURCE)
	$(CXX) $(CXXFLAGS) -DDEBUG -o $(TARGET) $<