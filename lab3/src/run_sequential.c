#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
  char **child_argv = malloc(sizeof(char *) * (argc + 1));
  if (!child_argv) {
    perror("malloc");
    return 1;
  }

  child_argv[0] = "./sequential_min_max";   // что запускаем
  for (int i = 1; i < argc; i++) {
    child_argv[i] = argv[i];                // пробрасываем аргументы
  }
  child_argv[argc] = NULL;                  // маркер конца argv

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    free(child_argv);
    return 1;
  }

  if (pid == 0) {
    execv("./sequential_min_max", child_argv);
    perror("execv");
    _exit(127);   // 127 — стандартный код "команда не найдена"
  }
  
  int status = 0;
  if (waitpid(pid, &status, 0) == -1) {
    perror("waitpid");
    free(child_argv);
    return 1;
  }

  free(child_argv);
  return 0;
}