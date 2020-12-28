/*
  Sends requested blocks from the input file repeatedly over a TCP socket.
  Uses mmap() + send().
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
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#include <array>
#include <cinttypes>
#include <thread>

#include <cppchannel/channel>

#include "flatbuffers/flatbuffers.h"
#include "log.h"
#include "wire.h"

#define bail(...)                                                              \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fprintf(stderr, "\n");                                                     \
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
constexpr size_t BLOCKSIZE = 64 * 1024;
constexpr int NUMBLOCKS = 64;

using Channel = cpp::channel<LReq, NUMBLOCKS>;

/* Receive requested read size from client, madvise() & send().
   Requests will be returned on the wire in the order they were received,
   but madvise() calls can be issued as soon as we receive a request. We'll have
   a single thread issuing send() calls on the socket but multiple threads
   can issue madvise() calls.
*/

using flatbuffers::uoffset_t;

void t_recv(void *fmap, int sock_fd, Channel &reqs, off_t filesize) {
  std::vector<uint8_t> reqBuf;

  uint64_t count = 0;
  while (true) {
    // read the size field
    std::array<uint8_t, sizeof(uoffset_t)> msgSizeBuf;
    ssize_t bytesRead = recv(sock_fd, msgSizeBuf.data(), msgSizeBuf.size(),
                             MSG_PEEK | MSG_WAITALL);
    if (bytesRead == -1) {
      pbail("recv");
    } else if (bytesRead != msgSizeBuf.size()) {
      bail("partial recv on req %" PRIu64 "; expected %zd, got %zd", count,
           msgSizeBuf.size(), bytesRead);
    }
    const uoffset_t msgSize =
        flatbuffers::ReadScalar<uoffset_t>(msgSizeBuf.data());
    DLOG("incoming message size: %d (not including %zd-byte prefix)\n", msgSize,
         sizeof(uoffset_t));
    const size_t totalSize = msgSize + sizeof(uoffset_t);
    DLOG("receiving %zd bytes (including %zd-byte prefix)\n", totalSize,
         sizeof(uoffset_t));
    reqBuf.resize(totalSize);
    bytesRead = recv(sock_fd, &reqBuf[0], reqBuf.size(), MSG_WAITALL);
    if (bytesRead == -1) {
      pbail("recv");
    } else if ((unsigned)bytesRead != reqBuf.size()) {
      bail("partial recv");
    }
    const auto *req = Server::GetSizePrefixedReq(reqBuf.data());
    flatbuffers::Verifier verifier(reqBuf.data(), reqBuf.size());
    if (!Server::VerifySizePrefixedReqBuffer(verifier)) {
      bail("invalid flatbuffer");
    }
    DLOG("req offset: 0x%" PRIx64 " size: 0x%" PRIx32 "\n", req->offset(),
         req->size());
    if (req->offset() + req->size() > filesize) {
      fprintf(stderr,
              "invalid read requested; filesize: %ld, offset: %" PRId64
              ", request size: %" PRIu32 "\n",
              filesize, req->offset(), req->size());
      continue;
    }
    if (madvise(static_cast<uint8_t*>(fmap) + req->offset(), req->size(), MADV_SEQUENTIAL)) {
      pbail("madvise");
    }
    LReq lreq{.offset = req->offset(), .size = req->size()};
    reqs.send(lreq);
    count++;
  }
}

void t_read(void *fmap, int sock_fd, Channel &reqs) {
  while (true) {
    auto req = reqs.recv();
    off_t offset = req.offset;
    size_t size = req.size;
    size_t remaining = req.size;
    while (remaining > 0) {
      ssize_t sent = send(sock_fd, static_cast<uint8_t*>(fmap) + offset, size, 0);
      if (sent == -1) {
        pbail("send failed");
      }
      remaining -= sent;
      offset += sent;
    }
  }
}

void serve(int socket_dest_fd, void *fmap, off_t filesize) {
  Channel reqs;
  std::thread reader(t_read, fmap, socket_dest_fd, std::ref(reqs));
  std::thread receiver(t_recv, fmap, socket_dest_fd, std::ref(reqs),
                       filesize);
  reader.join();
  receiver.join();
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

  void *fmap = mmap(nullptr, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (fmap == MAP_FAILED) {
    pbail("mmap failed");
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

    fprintf(stderr, "accepted\n");
    while (true) {
      serve(s_fd, fmap, statbuf.st_size);
    }
  }
  return 0;
}
