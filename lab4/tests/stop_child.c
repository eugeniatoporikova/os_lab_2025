#define _GNU_SOURCE
#include <dlfcn.h>
#include <signal.h>
#include <unistd.h>
pid_t fork(void) {
  pid_t (*real_fork)(void) = dlsym(RTLD_NEXT, "fork");
  pid_t pid = real_fork();
  if (pid == 0) raise(SIGSTOP);
  return pid;
}
