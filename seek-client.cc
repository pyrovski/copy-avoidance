/*
  Sends the input file repeatedly over a TCP socket.
*/

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <array>
#include <cinttypes>
#include <thread>

#include "flatbuffers/flatbuffers.h"
#include "log.h"
#include "wire.h"

#define bail(...)                                                              \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(1);                                                                   \
  } while (0)
#define pbail(...)                                                             \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    perror(" ");                                                               \
    exit(1);                                                                   \
  } while (0)

#define zero(_x) memset(&_x, 0, sizeof(_x))

const unsigned short PORT = 9999;
const char PORT_STR[] = "9999";
constexpr size_t BLOCKSIZE = 64 * 1024;
constexpr int NUMBLOCKS = 64;

// TODO: limit the number of outstanding requests
void t_req(int sfd) {
  constexpr uint64_t filesize = 1024 * 1024;
  off_t offset = 0;
  while (true) {
    // for (int i = 0; i < 10; ++i) {
    // TODO: re-use a buffer for each flatbuffer
    flatbuffers::FlatBufferBuilder fbb;
    auto req = Server::CreateReq(fbb, offset, BLOCKSIZE);
    fbb.FinishSizePrefixed(req);
    DLOG("req offset: 0x%" PRIx64 " size: 0x%" PRIx32 "\n", offset,
         (uint32_t)BLOCKSIZE);
    DLOG("sending %d bytes\n", fbb.GetSize());
    const auto bytesSent = send(sfd, fbb.GetBufferPointer(), fbb.GetSize(), 0);
    if (bytesSent != fbb.GetSize()) {
      pbail("send");
    }
    offset += BLOCKSIZE;
    if (offset + BLOCKSIZE > filesize) {
      DLOG("resetting offset\n");
      offset = 0;
    }
    fflush(stderr);
  }
}

void t_recv(int sfd) {
  while (true) {
    std::array<uint8_t, BLOCKSIZE> buf;
    recv(sfd, buf.data(), BLOCKSIZE, MSG_WAITALL);
  }
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);

  if (argc != 2) {
    bail("expected a hostname\n");
  }

  struct addrinfo hints;
  zero(hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *info;
  if (getaddrinfo(argv[1], PORT_STR, &hints, &info) != 0) {
    bail("getaddrinfo");
  }
  int sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (sfd == -1) {
    pbail("socket");
  }
  if (connect(sfd, info->ai_addr, info->ai_addrlen) == -1) {
    pbail("connect");
  }
  freeaddrinfo(info);
  info = nullptr;
  fprintf(stderr, "connected\n");

  std::thread requester(t_req, sfd);
  std::thread receiver(t_recv, sfd);
  requester.join();
  receiver.join();

  return 0;
}
