#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0: {
            char *endptr = NULL;
            long v = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || v < 0) {
              fprintf(stderr, "Invalid --seed value: %s\n", optarg);
              return 1;
            }
            seed = (int)v;
            break;
          }
          case 1: {
            char *endptr = NULL;
            long v = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || v <= 0) {
              fprintf(stderr, "Invalid --array_size value: %s\n", optarg);
              return 1;
            }
            array_size = (int)v;
            break;
          }
          case 2: {
            char *endptr = NULL;
            long v = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || v <= 0) {
              fprintf(stderr, "Invalid --pnum value: %s\n", optarg);
              return 1;
            }
            pnum = (int)v;
            break;
          }
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  if (!array) {
    perror("malloc");
    return 1;
  }
  GenerateArray(array, array_size, seed);

  int active_child_processes = 0;

  // --- Подготовка каналов/файлов ДО fork ---
  int (*pipes)[2] = NULL;   // массив pipe: pipes[i][0]/pipes[i][1]
  char **file_names = NULL; // имена временных файлов (режим файлов)

  if (!with_files) {
    pipes = malloc(sizeof(int[2]) * pnum);
    if (!pipes) { perror("malloc"); free(array); return 1; }
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        free(array);
        free(pipes);
        return 1;
      }
    }
  } else {
    file_names = malloc(sizeof(char *) * pnum);
    if (!file_names) { perror("malloc"); free(array); return 1; }
    for (int i = 0; i < pnum; i++) {
      file_names[i] = malloc(64);
      snprintf(file_names[i], 64, "/tmp/minmax_%d_%d.txt", getpid(), i);
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        // === Дочерний процесс ===

        // Разбиение диапазона [begin, end) для процесса i
        int begin = i * (array_size / pnum);
        int end = (i == pnum - 1) ? array_size
                                  : (i + 1) * (array_size / pnum);

        struct MinMax res = GetMinMax(array, begin, end);

        if (with_files) {
          // --- запись в файл ---
          FILE *f = fopen(file_names[i], "w");
          if (!f) {
            perror("fopen");
            _exit(1);
          }
          fprintf(f, "%d %d\n", res.min, res.max);
          fclose(f);
        } else {
          // --- запись в pipe ---
          // Закрываем все чужие pipe и читающие концы
          for (int j = 0; j < pnum; j++) {
            close(pipes[j][0]);
            if (j != i) close(pipes[j][1]);
          }
          // Пишем результат (два int)
          if (write(pipes[i][1], &res.min, sizeof(int)) != sizeof(int) ||
              write(pipes[i][1], &res.max, sizeof(int)) != sizeof(int)) {
            perror("write");
            _exit(1);
          }
          close(pipes[i][1]);
        }

        free(array);
        _exit(0);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // --- Родитель закрывает ненужные концы pipe ---
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]); // родитель не пишет
    }
  }

  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      FILE *f = fopen(file_names[i], "r");
      if (!f) {
        perror("fopen");
        continue;
      }
      if (fscanf(f, "%d %d", &min, &max) != 2) {
        fprintf(stderr, "Failed to read %s\n", file_names[i]);
        fclose(f);
        continue;
      }
      fclose(f);
      unlink(file_names[i]); // удаляем временный файл
    } else {
      if (read(pipes[i][0], &min, sizeof(int)) != sizeof(int) ||
          read(pipes[i][0], &max, sizeof(int)) != sizeof(int)) {
        perror("read");
        close(pipes[i][0]);
        continue;
      }
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  // --- Освобождение ресурсов ---
  if (with_files) {
    for (int i = 0; i < pnum; i++) free(file_names[i]);
    free(file_names);
  } else {
    free(pipes);
  }
  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
