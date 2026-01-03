GCC := gcc
FLAGS := -Wall -Wextra -Wshadow -pthread -pedantic
SRC := threadPool_test.c ../threadPool.c

.PHONY: clean

test:
	rm -rf test;
	$(GCC) $(FLAGS) $(SRC) -o test

memcheck:
	$(GCC) $(FLAGS) $(SRC) -o test;
	valgrind ./test --leak-check=full -s

clean:
	rm -rf test
