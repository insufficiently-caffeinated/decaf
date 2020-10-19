
#include "decaf.h"
#include <stdint.h>

void test(uint32_t x) {
  decaf_assume(x != 5);

  if (x != 5) {
    decaf_assume(true);
  } else {
    decaf_assert(false);
  }

  decaf_assert(true);
}
