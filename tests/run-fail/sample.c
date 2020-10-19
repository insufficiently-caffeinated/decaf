
#include "decaf.h"
#include <stdint.h>

void test(uint32_t x) {
  uint32_t a = x + 1020202;
  uint32_t b = x * 555142;
  uint32_t c = x - 1321;

  decaf_assert(x + 5 != 0);
}
