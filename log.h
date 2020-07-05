#ifndef LOG_H
#define LOG_H
#ifndef NDEBUG
#define DLOG(...)                                                              \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
  } while (0)
#else
#define DLOG(...)                                                              \
  do {                                                                         \
  } while (0)
#endif
#define DL() fprintf(stderr, "%ld: %d\n", syscall(__NR_gettid), __LINE__)
#endif
