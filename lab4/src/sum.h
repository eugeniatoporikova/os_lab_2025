#ifndef SUM_H
#define SUM_H
#include <stdint.h>
struct SumArgs {
  const int *array;
  int begin;
  int end;
  int64_t result;
};
int64_t Sum(const struct SumArgs *args);
#endif
