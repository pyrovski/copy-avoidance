/*
  Sends the input file repeatedly over a TCP socket.
*/

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
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

#include <boost/fiber/buffered_channel.hpp>

#include "wire.h"

#define bail(...) do{fprintf(stderr, __VA_ARGS__); exit(1);} while(0)
#define pbail(...) do{fprintf(stderr, __VA_ARGS__); perror(" "); exit(1);} while(0)

#define zero(_x) memset(&_x, 0, sizeof(_x))
#define DL() fprintf(stderr, "%ld: %d\n", syscall(__NR_gettid), __LINE__)

const unsigned short PORT = 9999;
constexpr size_t BLOCKSIZE = 64 * 1024;
constexpr int NUMBLOCKS = 64;

using Channel = boost::fibers::buffered_channel < Req >;

/* TODO: receive requested read size from client, fadvise, read & send via 
   sendfile().
   Requests will be returned on the wire in the order they were received,
   but fadvise() calls can be issued as soon as we receive a request. We'll have
   a single thread issuing sendfile() calls on the socket but multiple threads
   can issue fadvise() calls.
*/

void t_recv(int fd, int sock_fd, Channel & reqs, off_t filesize) {
    while (true) {
        Req req;
        ssize_t bytesRead = recv(sock_fd, &req, sizeof(req), MSG_WAITALL);
        if (bytesRead != sizeof(req)) {
            bail("partial recv");
        }
        if (req.offset + req.size > filesize) {
            fprintf(stderr, "invalid read requested");
            continue;
        }
        // TODO: evaluate POSIX_FADV_WILLNEED
        if (posix_fadvise(fd, req.offset, req.size, POSIX_FADV_SEQUENTIAL)) {
            pbail("fadvise");
        }
        reqs.push(req);
    }
}

void t_read(int fd, int sock_fd, Channel & reqs) {
 for (auto req:reqs) {
        off_t offset = req.offset;
        size_t size = req.size;
        size_t remaining = req.size;
        while (remaining > 0) {
            ssize_t sent = sendfile(sock_fd, fd, &offset, size);
            if (sent == -1) {
                pbail("sendfile failed");
            }
            remaining -= sent;
            offset += sent;
        }
    }
}

void serve(int socket_dest_fd, int src_fd, off_t filesize) {
    Channel reqs(NUMBLOCKS);
    std::thread reader(t_read, src_fd, socket_dest_fd, std::ref(reqs));
    std::thread receiver(t_recv, src_fd, socket_dest_fd, std::ref(reqs),
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
            serve(s_fd, fd, statbuf.st_size);
        }
    }
    return 0;
}
