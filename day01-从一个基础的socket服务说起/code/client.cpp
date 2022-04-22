#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <cstdio>
#include <iostream>

int main() {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(8888);
  if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    printf("connect err: %s\n", strerror(errno));
    return -1;
  };
  
  printf("success connect to server\n");
  char input_msg[100];
  std::cin >> input_msg;
  printf("input_msg: %s\n", input_msg);
  int write_num = write(sock_fd, input_msg, 100);
  char read_msg[100];
  int read_num = read(sock_fd, read_msg, 100);
  printf("get from server: %s\n", read_msg);
  close(sock_fd);
}