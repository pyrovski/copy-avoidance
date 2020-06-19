#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#define bail(...) do{fprintf(stderr, __VA_ARGS__); return 1;} while(0)
#define pbail(...) do{fprintf(stderr, __VA_ARGS__); perror(" "); return 1;} while(0)

#define zero(_x) memset(&_x, 0, sizeof(_x))

const unsigned short PORT = 9999;

int main(int argc, char ** argv) {
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

  if (bind(sock, (struct sockaddr*)&s_addr, sizeof(s_addr)) == -1) {
    pbail("bind failed");
  }

  if (listen(sock, 0) == -1) {
    pbail("listen failed");
  }

  while (true) {
    socklen_t so_size = sizeof(s_addr);
    int s_fd = accept(sock, (struct sockaddr *)&s_addr, &so_size);
    if (s_fd == -1) {
      pbail("accept failed");
    }

    off_t offset = 0;
    ssize_t sent = sendfile(s_fd, fd, &offset, statbuf.st_size);
    if (sent == -1) {
      pbail("sendfile failed");
    }
    printf("sent %zd\n", sent);
  }
  return 0;
}
