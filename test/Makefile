GCC := gcc
FLAGS := -Wall -Wextra -Wshadow -pthread -pedantic
SRC := ../src/
VALGRIND := valgrind --leak-check=full -s

.PHONY: clean

test-tp:
	rm -rf tp-test;
	$(GCC) $(FLAGS) $(SRC)*.c threadPool_test.c -o tp-test

test-rb:
	rm -rf rb-test;
	$(GCC) $(FLAGS) $(SRC)xmalloc.c $(SRC)ringBuffer.c ringBuffer_test.c -o rb-test

memcheck:
	$(GCC) $(FLAGS) $(SRC)*.c threadPool_test.c -o tp-test;
	$(GCC) $(FLAGS) $(SRC)xmalloc.c $(SRC)ringBuffer.c ringBuffer_test.c -o rb-test;
	$(VALGRIND) ./tp-test 100 100;
	$(VALGRIND) ./rb-test;

clean:
	rm -rf tp-test rb-test;
