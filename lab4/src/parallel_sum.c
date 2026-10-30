#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sum.h"
#include "utils.h"

static void *ThreadSum(void *arg) {
  struct SumArgs *args = arg;
  args->result = Sum(args);
  return NULL;
}

int main(int argc, char **argv) {
  int threads_num = -1, seed = -1, array_size = -1;
  const struct option options[] = {
    {"threads_num", required_argument, NULL, 't'},
    {"seed", required_argument, NULL, 's'},
    {"array_size", required_argument, NULL, 'a'},
    {NULL, 0, NULL, 0}
  };
  int option;
  while ((option = getopt_long(argc, argv, "", options, NULL)) != -1) {
    if (option == '?') return 1;
    char *end;
    errno = 0;
    long value = strtol(optarg, &end, 10);
    if (errno || end == optarg || *end || value > INT_MAX ||
        value < (option == 's' ? 0 : 1)) {
      fprintf(stderr, "Invalid argument: %s\n", optarg);
      return 1;
    }
    if (option == 't') threads_num = (int)value;
    if (option == 's') seed = (int)value;
    if (option == 'a') array_size = (int)value;
  }
  if (optind != argc || threads_num < 1 || array_size < 1 || seed < 0) {
    fprintf(stderr, "Usage: %s --threads_num N --seed N --array_size N\n", argv[0]);
    return 1;
  }
  int *array = malloc(sizeof(*array) * array_size);
  pthread_t *threads = malloc(sizeof(*threads) * threads_num);
  struct SumArgs *args = calloc(threads_num, sizeof(*args));
  if (!array || !threads || !args) {
    perror("malloc");
    free(array); free(threads); free(args);
    return 1;
  }
  GenerateArray(array, array_size, seed);
  struct timespec start, finish;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int created = 0, failed = 0;
  for (int i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = (int)((int64_t)i * array_size / threads_num);
    args[i].end = (int)(((int64_t)i + 1) * array_size / threads_num);
    int error = pthread_create(&threads[i], NULL, ThreadSum, &args[i]);
    if (error) {
      fprintf(stderr, "pthread_create: %s\n", strerror(error));
      failed = 1;
      break;
    }
    created++;
  }
  int64_t total = 0;
  for (int i = 0; i < created; i++) {
    int error = pthread_join(threads[i], NULL);
    if (error) {
      fprintf(stderr, "pthread_join: %s\n", strerror(error));
      exit(1);
    }
    total += args[i].result;
  }
  clock_gettime(CLOCK_MONOTONIC, &finish);
  double elapsed = (finish.tv_sec - start.tv_sec) * 1000.0 +
                   (finish.tv_nsec - start.tv_nsec) / 1000000.0;
  free(array); free(threads); free(args);
  if (failed) return 1;
  printf("Total: %" PRId64 "\n", total);
  printf("Elapsed time: %.6fms\n", elapsed);
  return 0;
}
