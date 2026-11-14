CC = gcc
CFLAGS = -Wall -pthread

all: parallel_min_max process_memory zombie psum

parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) $^ -o $@

process_memory: process_memory.o
	$(CC) $^ -o $@

zombie: zombie.o
	$(CC) $^ -o $@

psum: parallel_sum.o sum.o utils.o
	$(CC) $^ -o $@

# --- Зависимости объектных файлов ---
# (Make сам применит правило: gcc $(CFLAGS) -c файл.c -o файл.o)
parallel_min_max.o: find_min_max.h utils.h
utils.o: utils.h
find_min_max.o: find_min_max.h utils.h
process_memory.o:
zombie.o:
sum.o: sum.h
parallel_sum.o: sum.h utils.h

# --- Очистка ---
clean:
	rm -f *.o parallel_min_max process_memory zombie psum
