#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // Для функции sleep()

// Создаем два мьютекса
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Функция первого потока
void* thread1_routine(void* args) {
    printf("Поток 1: попытка захватить mutex1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Поток 1: mutex1 захвачен. Выполняю работу...\n");

    // Искусственная задержка, чтобы второй поток успел захватить свой мьютекс
    sleep(1);

    printf("Поток 1: попытка захватить mutex2...\n");
    // Здесь Поток 1 заблокируется, так как mutex2 уже захвачен Потоком 2
    pthread_mutex_lock(&mutex2); 
    
    printf("Поток 1: mutex2 захвачен! (Эта строка никогда не выведется)\n");

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

// Функция второго потока
void* thread2_routine(void* args) {
    printf("Поток 2: попытка захватить mutex2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Поток 2: mutex2 захвачен. Выполняю работу...\n");

    // Искусственная задержка, чтобы первый поток успел захватить свой мьютекс
    sleep(1);

    printf("Поток 2: попытка захватить mutex1...\n");
    // Здесь Поток 2 заблокируется, так как mutex1 уже захвачен Потоком 1
    pthread_mutex_lock(&mutex1); 
    
    printf("Поток 2: mutex1 захвачен! (Эта строка никогда не выведется)\n");

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    printf("Запуск программы демонстрации Deadlock...\n");

    // Создаем потоки
    pthread_create(&thread1, NULL, thread1_routine, NULL);
    pthread_create(&thread2, NULL, thread2_routine, NULL);

    // Ожидаем завершения (программа зависнет на этом этапе)
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Программа успешно завершена. (Эта строка никогда не выведется)\n");

    return 0;
}