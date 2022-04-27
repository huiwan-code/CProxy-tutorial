#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <arpa/inet.h>
#include <string.h>
#include <cstdio>
#include <errno.h>
#include <vector>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>

int setfdNonBlock(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) return -1;
  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1) return -1;
  return 0;
};

void handleConn(int accept_fd) {
  char read_msg[100];
  char *buf_ptr = read_msg;
  int total_read_num = 0;
  int read_num = 0;
  // 使用的是epollet边缘触发模式，需要把套接字缓存区中的数据全读完
  do {
    read_num = read(accept_fd, buf_ptr, 100);
    buf_ptr += read_num;
    total_read_num += read_num;
  } while(read_num > 0);
  printf("get msg from client: %s\n", read_msg);
  int write_num = write(accept_fd, read_msg, total_read_num);
  close(accept_fd);
}

int listenServer(char *host, int port) {
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
  return listen_fd;
}

const int EPOLLWAIT_TIME = 10000;
const int EVENTSMAXNUM = 4096;

class HandleThread {
  public:
    HandleThread() 
    : epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
      epoll_events_(EVENTSMAXNUM),
      thread_(std::bind(&HandleThread::work, this)) {
      assert(epoll_fd_ > 0);
      thread_.detach();
    }
    ~HandleThread() {
      close(epoll_fd_);
    }
    // 线程实际运行函数
    void work();
    // 添加监听套接字
    void addFd(int fd);
    // 不再监听指定套接字
    void rmFd(int fd);
  private:
    int epoll_fd_;
    std::vector<epoll_event>epoll_events_;
    std::thread thread_;
};

void HandleThread::work() {
  for(;;) {
    int event_count = epoll_wait(epoll_fd_, &*epoll_events_.begin(), epoll_events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0) {
      perror("epoll wait error");
      continue;
    }
    for (int i = 0; i < event_count; i++) {
      epoll_event cur_event = epoll_events_[i];
      int fd = cur_event.data.fd;

      // 不再监听fd，从epoll中去掉
      rmFd(fd);
      // 处理连接读写
      handleConn(fd);
    }
  }
}

void HandleThread::addFd(int fd) {
  epoll_event event;
  event.data.fd = fd;
  // 只监听读事件
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    perror("epoll_add error");
  }
}

void HandleThread::rmFd(int fd) {
  epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    perror("epoll_del error");
  }
}

typedef std::shared_ptr<HandleThread> SP_HandleThread;

class HandleThreadPool {
  public:
    HandleThreadPool(int thread_nums) : thread_nums_(thread_nums), next_thread_idx_(0) {
      for (int i = 0; i < thread_nums; i++) {
        SP_HandleThread t (new HandleThread());
        thread_pool_.push_back(t);
      }
    }
    SP_HandleThread getThread();
  private:
    int thread_nums_;
    int next_thread_idx_;
    std::vector<SP_HandleThread> thread_pool_;
};

// 从线程池中获取一个线程
SP_HandleThread HandleThreadPool::getThread() {
  SP_HandleThread t = thread_pool_[next_thread_idx_];
  next_thread_idx_ = (next_thread_idx_ + 1) % thread_nums_;
  return t;
}

int main() {
  int listen_fd = listenServer("127.0.0.1", 8888);

  // 创建线程池
  HandleThreadPool pool(4);
  // 等待1秒
  sleep(1);
  struct sockaddr_in client_addr;
  bzero(&client_addr, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  while((accept_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len)) > 0) {
    printf("get accept_fd: %d from: %s:%d\n", accept_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    // 将fd设置为非阻塞 ?
    setfdNonBlock(accept_fd);
    // 从pool中获取一个线程处理连接
    SP_HandleThread t = pool.getThread();
    t->addFd(accept_fd);
  }
}

