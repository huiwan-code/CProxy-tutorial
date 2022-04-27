#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <arpa/inet.h>
#include <string.h>
#include <cstdio>
#include <errno.h>

void handleConn(int accept_fd) {
  char read_msg[100];
  int read_num = read(accept_fd, read_msg, 100);
  printf("get msg from client: %s\n", read_msg);
  int write_num = write(accept_fd, read_msg, read_num);
  close(accept_fd);
}

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(8888);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    printf("bind err: %s\n", strerror(errno));
    close(listen_fd);
    return -1;
  }

  if (listen(listen_fd, 2048) < 0) {
    printf("listen err: %s\n", strerror(errno));
    close(listen_fd);
    return -1;
  }
  
  struct sockaddr_in client_addr;
  bzero(&client_addr, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  while((accept_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len)) > 0) {
    printf("get accept_fd: %d from: %s:%d\n", accept_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    std::thread handleThread(handleConn, accept_fd);
    // 将线程设置为后台线程，避免阻塞主线程
    handleThread.detach();
  }
}

