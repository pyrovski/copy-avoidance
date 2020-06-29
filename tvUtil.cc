#include <sys/time.h>
#include <time.h>
#include "tvUtil.h"

// subtract timespec rhs from lhs
struct timespec tsDiff(const struct timespec &lhs, struct timespec &rhs) {
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

double tsDouble(const struct timespec &tv) {
    return tv.tv_sec + tv.tv_nsec / 1000000000.0;
}

// subtract timeval rhs from lhs
struct timeval tvDiff(const struct timeval &lhs, struct timeval &rhs) {
    struct timeval dest;
    /* Perform the carry for the later subtraction by updating rhs */
    if (lhs.tv_usec < rhs.tv_usec) {
        int usec = (rhs.tv_usec - lhs.tv_usec) / 1000000 + 1;
        rhs.tv_usec -= 1000000 * usec;
        rhs.tv_sec += usec;
    }
    if (lhs.tv_usec - rhs.tv_usec > 1000000) {
        int usec = (lhs.tv_usec - rhs.tv_usec) / 1000000;
        rhs.tv_usec += 1000000 * usec;
        rhs.tv_sec -= usec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    dest.tv_sec = lhs.tv_sec - rhs.tv_sec;
    dest.tv_usec = lhs.tv_usec - rhs.tv_usec;

    return dest;
}

double tvDouble(const struct timeval &tv) {
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
