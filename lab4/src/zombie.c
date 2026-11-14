#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork(); 

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        printf("Ребенок (PID %d) родился и сейчас умрет.\n", getpid());
        exit(0);
    } 
    else {
        printf("Родитель (PID %d) создал ребенка (PID %d)\n", getpid(), pid);
        printf("Родитель НЕ вызывает wait(). Ребенок станет зомби\n");
        printf("Родитель будет спать 60 секунд\n");
        
        sleep(60); 
        
        printf("Родитель проснулся и завершается.\n");
    }
    printf("fdff");
    return 0;
}
