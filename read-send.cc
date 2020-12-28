/*
  Sends the entire input file repeatedly over a TCP socket, using read() +
  send().
*/

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <array>

#include "tvUtil.h"

#define bail(...)                 \
  do {                            \
    fprintf(stderr, __VA_ARGS__); \
    exit(1);                      \
  } while (0)
#define pbail(...)                \
  do {                            \
    fprintf(stderr, __VA_ARGS__); \
    perror(" ");                  \
    exit(1);                      \
  } while (0)

#define zero(_x) memset(&_x, 0, sizeof(_x))

const unsigned short PORT = 9999;
constexpr uint32_t blocksize = 64 * 1024;

// TODO: parallelize, pipeline
// Send `count` bytes from `src_fd` to `socket_dest_fd`, starting from `offset`.
ssize_t sendfile(int socket_dest_fd, int src_fd, off_t *offset, size_t count) {
  fprintf(stderr, "naive method\n");
  if (offset != nullptr && lseek(src_fd, *offset, SEEK_SET) == -1) {
    pbail("lseek failed");
  }
  std::array<uint8_t, blocksize> buf;
  ssize_t result = 0;
  while (count > 0) {
    ssize_t bytes_read = read(src_fd, buf.data(), blocksize);
    if (bytes_read == -1) {
      pbail("read failed");
    }
    count -= bytes_read;
    size_t remaining = bytes_read;
    size_t sent = 0;
    while (remaining > 0) {
      ssize_t bytes_sent =
          send(socket_dest_fd, buf.data() + sent, bytes_read - sent, 0);
      if (bytes_sent == -1) {
        pbail("send failed");
      }
      sent += bytes_sent;
      remaining -= bytes_sent;
    }
    result += bytes_read;
  }
  return result;
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);

  if (argc != 2) {
    bail("expected a file path\n");
  }
  const int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    pbail("open failed");
  }

  struct stat statbuf;
  zero(statbuf);
  if (fstat(fd, &statbuf)) {
    pbail("fstat failed");
  }

  const int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    pbail("socket  failed");
  }
  const int option = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  struct sockaddr_in s_addr;
  zero(s_addr);
  s_addr.sin_family = AF_INET;
  s_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  s_addr.sin_port = htons(PORT);

  if (bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1) {
    pbail("bind failed");
  }

  if (listen(sock, 0) == -1) {
    pbail("listen failed");
  }
  struct rusage usage1, usage2;

  while (true) {
    socklen_t so_size = sizeof(s_addr);
    printf("waiting for connections\n");
    int s_fd = accept(sock, (struct sockaddr *)&s_addr, &so_size);
    if (s_fd == -1) {
      pbail("accept failed");
    }

    while (true) {
      printf("sending %s\n", argv[1]);
      off_t offset = 0;
      struct timespec ts_start;
      if (getrusage(RUSAGE_SELF, &usage1) == -1) {
        pbail("getrusage failed");
      }
      clock_gettime(CLOCK_MONOTONIC, &ts_start);
      ssize_t sent = sendfile(s_fd, fd, &offset, statbuf.st_size);
      struct timespec ts_end;
      clock_gettime(CLOCK_MONOTONIC, &ts_end);
      if (getrusage(RUSAGE_SELF, &usage2) == -1) {
        pbail("getrusage failed");
      }
      if (sent == -1) {
        perror("sendfile failed");
        break;
      }
      const float elapsed = tsDouble(tsDiff(ts_end, ts_start));
      printf("sent %zd bytes in %fs; %f MiB/s; user: %fs; system: %fs\n", sent,
             elapsed, sent / 1024 / 1024 / elapsed,
             tvDouble(tvDiff(usage2.ru_utime, usage1.ru_utime)),
             tvDouble(tvDiff(usage2.ru_stime, usage1.ru_stime)));
    }
  }
  return 0;
}
