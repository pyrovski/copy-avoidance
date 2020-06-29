/*
  Sends the input file repeatedly over a TCP socket.
*/

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
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
const char PORT_STR[] = "9999";
constexpr size_t BLOCKSIZE = 64 * 1024;
constexpr int NUMBLOCKS = 64;

using Channel = boost::fibers::buffered_channel < Req >;

void t_req(int sfd) {
    constexpr uint64_t filesize = 1024 * 1024;
    off_t offset = 0;
    Req req;
    req.size = BLOCKSIZE;
    req.offset = offset;
    while (true) {
        send(sfd, &req, sizeof(req), 0);
        req.offset += BLOCKSIZE;
        if (req.offset + req.size > filesize) {
            req.offset = 0;
        }
    }
}

void t_recv(int sfd) {
    while (true) {
        std::array < uint8_t, BLOCKSIZE > buf;
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
