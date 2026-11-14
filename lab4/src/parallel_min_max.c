#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>   
#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"


static pid_t *child_pids;
static int pnum_global;


static void on_alarm(int sig) {
  (void)sig;
  for (int i = 0; i < pnum_global; i++) {
    kill(child_pids[i], SIGKILL); 
  }
}

int main(int argc, char **argv) {
  int seed = -1, array_size = -1, pnum = -1, timeout = 0;
  bool with_files = false;


  static struct option opts[] = {
      {"seed",       required_argument, 0, 's'},
      {"array_size", required_argument, 0, 'a'},
      {"pnum",       required_argument, 0, 'p'},
      {"timeout",    required_argument, 0, 't'},
      {"by_files",   no_argument,       0, 'f'},
      {0, 0, 0, 0}};

  int c;
  while ((c = getopt_long(argc, argv, "f", opts, NULL)) != -1) {
    switch (c) {
      case 's': seed = atoi(optarg); break;
      case 'a': array_size = atoi(optarg); break;
      case 'p': pnum = atoi(optarg); break;
      case 't': timeout = atoi(optarg); break;
      case 'f': with_files = true; break;
      default: return 1;
    }
  }

  if (seed <= 0 || array_size <= 0 || pnum <= 0) {
    fprintf(stderr, "Usage: %s --seed N --array_size N --pnum N "
                    "[--timeout N] [--by_files]\n", argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  child_pids = malloc(sizeof(pid_t) * pnum);
  pnum_global = pnum;

  int (*pipes)[2] = NULL;
  char **files = NULL;
  
  if (with_files) {
    files = malloc(sizeof(char *) * pnum);
    for (int i = 0; i < pnum; i++) {
      files[i] = malloc(64);
      snprintf(files[i], 64, "/tmp/minmax_%d_%d", getpid(), i);
    }
  } else {
    pipes = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; i++) {
      pipe(pipes[i]);
    }
  }

  if (timeout > 0) {
    signal(SIGALRM, on_alarm);
    alarm(timeout);
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    int begin = i * array_size / pnum;
    int end   = (i == pnum - 1) ? array_size : (i + 1) * array_size / pnum;

    pid_t pid = fork();
    if (pid == 0) { 
      struct MinMax r = GetMinMax(array, begin, end);

      if (with_files) {
        FILE *f = fopen(files[i], "w");
        fprintf(f, "%d %d\n", r.min, r.max);
        fclose(f);
      } else {
        for (int j = 0; j < pnum; j++) {
          close(pipes[j][0]);
          if (j != i) close(pipes[j][1]);
        }
        write(pipes[i][1], &r.min, sizeof(int));
        write(pipes[i][1], &r.max, sizeof(int));
        close(pipes[i][1]);
      }
      _exit(0); 
    }
    child_pids[i] = pid; 
  }

  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[i][1]);
    }
  }

  struct MinMax mm = {INT_MAX, INT_MIN};
  for (int i = 0; i < pnum; i++) {
    int mn, mx;
    if (with_files) {
      waitpid(child_pids[i], NULL, 0); 
      FILE *f = fopen(files[i], "r");
      fscanf(f, "%d %d", &mn, &mx);
      fclose(f);
      unlink(files[i]); 
      free(files[i]);
    } else {
      read(pipes[i][0], &mn, sizeof(int));
      read(pipes[i][0], &mx, sizeof(int));
      close(pipes[i][0]);
    }
    
    if (mn < mm.min) mm.min = mn;
    if (mx > mm.max) mm.max = mx;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(pipes);
  free(files);
  free(child_pids);
  free(array);

  printf("Min: %d\nMax: %d\n", mm.min, mm.max);
  printf("Elapsed time: %f ms\n", elapsed_time);

  return 0;
}
