CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -O2
TARGET = heap_tracker
SOURCE = heap_tracker.c

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	@echo "Creating simple test program..."
	@echo '#include <stdlib.h>' > simple_test.c
	@echo '#include <stdio.h>' >> simple_test.c
	@echo 'int main() {' >> simple_test.c
	@echo '    printf("Allocating memory...\\n");' >> simple_test.c
	@echo '    void *p1 = malloc(100);' >> simple_test.c
	@echo '    void *p2 = malloc(200);' >> simple_test.c
	@echo '    free(p1);' >> simple_test.c
	@echo '    printf("Test completed (p2 leaked)\\n");' >> simple_test.c
	@echo '    return 0;' >> simple_test.c
	@echo '}' >> simple_test.c
	@gcc -o simple_test simple_test.c
	@echo "Running heap tracker test:"
	@./$(TARGET) -l heap_test.log ./simple_test
	@echo "Log contents:"
	@cat heap_test.log
	@rm -f simple_test simple_test.c heap_test.log

clean:
	rm -f $(TARGET) simple_test simple_test.c heap_test.log

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/