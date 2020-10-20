
#include "decaf.h"
#include <stdint.h>

void test(int32_t x) {
  decaf_assume(x == -50);
  int32_t y = x / -5;
  decaf_assert(y == 10);
}
