#ifndef TVUTIL_H
#define TVUTIL_H
#include <time.h>

double tvDouble(const struct timespec &ts);
// Modifies rhs
struct timespec tvDiff(const struct timespec &lhs, struct timespec &rhs);
#endif
