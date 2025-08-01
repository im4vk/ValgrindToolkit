CC = gcc
CFLAGS = -Wall -Wextra -g -fPIC -O2 -std=c99
LDFLAGS = -shared -ldl -lpthread

TARGET = libmemtrack.so
SOURCE = memory_tracker.c
WRAPPER = memtrack

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

test: $(TARGET)
	@echo "Running memory tracker tests..."
	@echo "Creating test program..."
	@echo '#include <stdlib.h>' > test_program.c
	@echo '#include <stdio.h>' >> test_program.c
	@echo 'int main() {' >> test_program.c
	@echo '    void *p1 = malloc(100);' >> test_program.c
	@echo '    void *p2 = malloc(200);' >> test_program.c
	@echo '    free(p1);' >> test_program.c
	@echo '    /* p2 is intentionally leaked */' >> test_program.c
	@echo '    printf("Test program completed\\n");' >> test_program.c
	@echo '    return 0;' >> test_program.c
	@echo '}' >> test_program.c
	@gcc -o test_program test_program.c
	@echo "Running test with memory tracker:"
	@./$(WRAPPER) ./test_program
	@rm -f test_program test_program.c

clean:
	rm -f $(TARGET) test_program test_program.c

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/lib/
	install -m 755 $(WRAPPER) /usr/local/bin/

debug: $(SOURCE)
	$(CC) $(CFLAGS) -DDEBUG $(LDFLAGS) -o $(TARGET) $<