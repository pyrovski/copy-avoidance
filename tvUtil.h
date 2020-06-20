#ifndef TVUTIL_H
#define TVUTIL_H
#include <sys/time.h>
#include <time.h>

double tsDouble(const struct timespec &ts);
// Modifies rhs
struct timespec tsDiff(const struct timespec &lhs, struct timespec &rhs);

double tvDouble(const struct timeval &ts);
// Modifies rhs
struct timeval tvDiff(const struct timeval &lhs, struct timeval &rhs);
#endif
