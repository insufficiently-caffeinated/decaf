
#include "test-common.hpp"

int main(int argc, char **argv) {
  CountingFailureTracker tracker;

  run_with_tracker(argc, argv, &tracker);

  if (tracker.count == 0)
    return 1;
  return 0;
}
