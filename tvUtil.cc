#include <time.h>
#include "tvUtil.h"

// subtract timespec rhs from lhs
struct timespec tvDiff(const struct timespec &lhs, struct timespec &rhs){
  struct timespec dest;
  /* Perform the carry for the later subtraction by updating rhs */
  if (lhs.tv_nsec < rhs.tv_nsec) {
    int nsec = (rhs.tv_nsec - lhs.tv_nsec) / 1000000000 + 1;
    rhs.tv_nsec -= 1000000000 * nsec;
    rhs.tv_sec += nsec;
  }
  if (lhs.tv_nsec - rhs.tv_nsec > 1000000000) {
    int nsec = (lhs.tv_nsec - rhs.tv_nsec) / 1000000000;
    rhs.tv_nsec += 1000000000 * nsec;
    rhs.tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_nsec is certainly positive. */
  dest.tv_sec = lhs.tv_sec - rhs.tv_sec;
  dest.tv_nsec = lhs.tv_nsec - rhs.tv_nsec;

  return dest;
}

double tvDouble(const struct timespec &tv){
  double result = tv.tv_sec + tv.tv_nsec / 1000000000.0;
  return result;
}
