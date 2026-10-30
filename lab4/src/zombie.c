#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  pid_t child = fork();
  if (child < 0) { perror("fork"); return 1; }
  if (child == 0) _exit(0);
  printf("Parent PID: %ld, child PID: %ld\n", (long)getpid(), (long)child);
  /* WNOWAIT leaves the terminated child in the zombie state. */
  siginfo_t info;
  while (waitid(P_PID, child, &info, WEXITED | WNOWAIT) < 0) {
    if (errno != EINTR) { perror("waitid"); return 1; }
  }
  printf("Before waitpid: child exited, status not collected (zombie).\n");
  printf("waitid: PID=%ld, exit status=%d\n", (long)info.si_pid, info.si_status);
  fflush(stdout);
  sleep(5); /* Time to inspect the child with ps in another terminal. */
  while (waitpid(child, NULL, 0) < 0) {
    if (errno != EINTR) { perror("waitpid"); return 1; }
  }
  if (waitpid(child, NULL, WNOHANG) == -1 && errno == ECHILD) {
    printf("After waitpid: ECHILD, child removed.\n");
    return 0;
  }
  fprintf(stderr, "Unexpected child state\n");
  return 1;
}
