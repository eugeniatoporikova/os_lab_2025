CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
AR = ar

.PHONY: all clean
all: parallel_min_max process_memory zombie psum

parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) $(CFLAGS) $^ -o $@

process_memory: process_memory.o
	$(CC) $(CFLAGS) $^ -o $@

zombie: zombie.o
	$(CC) $(CFLAGS) $^ -o $@

libsum.a: sum.o
	$(AR) rcs $@ $^

psum: parallel_sum.o utils.o libsum.a
	$(CC) $(CFLAGS) parallel_sum.o utils.o -L. -lsum -pthread -o $@

parallel_sum.o: parallel_sum.c sum.h utils.h
	$(CC) $(CFLAGS) -pthread -c $< -o $@

sum.o: sum.c sum.h
utils.o: utils.c utils.h
find_min_max.o: find_min_max.c find_min_max.h
parallel_min_max.o: parallel_min_max.c find_min_max.h utils.h

clean:
	rm -f *.o *.a parallel_min_max process_memory zombie psum
