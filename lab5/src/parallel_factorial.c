#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

// Глобальные переменные для хранения промежуточного результата и модуля
unsigned long long global_result = 1;
unsigned long long mod = 1;

// Мьютекс для защиты записи в global_result
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

// Структура для передачи аргументов в поток
struct ThreadArgs {
    int start;
    int end;
};

// Функция, которую выполняет каждый поток
void* factorial_part(void* args) {
    struct ThreadArgs* targs = (struct ThreadArgs*)args;
    
    // Если диапазон некорректный (например, потоков больше, чем k), просто выходим
    if (targs->start > targs->end) {
        return NULL;
    }

    unsigned long long local_result = 1;
    
    // Локально вычисляем частичный факториал по модулю
    for (int i = targs->start; i <= targs->end; i++) {
        local_result = (local_result * i) % mod;
    }

    // Критическая секция: умножаем глобальный результат на локальный
    pthread_mutex_lock(&mut);
    global_result = (global_result * local_result) % mod;
    pthread_mutex_unlock(&mut);

    return NULL;
}

int main(int argc, char **argv) {
    int k = -1;
    int pnum = -1;
    
    // Настройка getopt_long для парсинга аргументов
    static struct option long_options[] = {
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    
    // Чтение аргументов командной строки: -k <val> --pnum=<val> --mod=<val>
    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = atoll(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -k <num> --pnum=<num> --mod=<num>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Проверка, что все аргументы были переданы
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Ошибка: Все параметры (-k, --pnum, --mod) должны быть больше 0.\n");
        exit(EXIT_FAILURE);
    }

    // Выделение памяти под потоки и их аргументы
    pthread_t* threads = (pthread_t*)malloc(pnum * sizeof(pthread_t));
    struct ThreadArgs* args = (struct ThreadArgs*)malloc(pnum * sizeof(struct ThreadArgs));

    // Вычисление размера "порции" вычислений для каждого потока
    int chunk_size = k / pnum;

    // Создание потоков
    for (int i = 0; i < pnum; i++) {
        args[i].start = i * chunk_size + 1;
        // Последний поток забирает остаток, если k не делится нацело на pnum
        args[i].end = (i == pnum - 1) ? k : (i + 1) * chunk_size;

        if (pthread_create(&threads[i], NULL, factorial_part, (void*)&args[i]) != 0) {
            perror("Ошибка при создании потока");
            exit(EXIT_FAILURE);
        }
    }

    // Ожидание завершения всех потоков (join)
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка при ожидании потока");
            exit(EXIT_FAILURE);
        }
    }

    printf("%d! mod %llu = %llu\n", k, mod, global_result);

    // Очистка ресурсов
    free(threads);
    free(args);
    pthread_mutex_destroy(&mut);

    return 0;
}