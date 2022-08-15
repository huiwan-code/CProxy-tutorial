## 教程说明
[C++高性能网络服务保姆级教程](https://zhuanlan.zhihu.com/p/500269188)
## 首发地址
[day04 高性能服务设计思路](https://mp.weixin.qq.com/s?__biz=MzI4MTc0NDg2OQ==&amp;mid=2247484712&amp;idx=1&amp;sn=c6debd08b853be0fdaca3c2346b79952&amp;chksm=eba5c1a2dcd248b447a22b6498b98c4f4462532f54e096f643714b37706275a834cdf93bb8a0&token=1198088579&lang=zh_CN#rd)

经过前面3节，我们已经具备了开发一个高性能服务的基础知识，并且还能搭建一个比较舒服的C++开发环境。

从这一节开始，将正式开始CProxy的开发。
## CProxy项目介绍

首先，先明确下我们的项目究竟是能干嘛？所谓内网穿透，简单讲就是在通过内网穿透工具(CProxy), 可以让局域网的服务(LocalServer)被公网(PublicClient)访问到。

原理说起来也简单，CProxy本身有一个公网ip，LocalServer注册到CProxy上，PublicClient访问CProxy的公网ip和端口，之后CProxy再将数据转发到LocalServer，下图是整体访问的数据流
![image.png](https://cdn.nlark.com/yuque/0/2022/png/1285552/1660059307217-65733d3d-27f0-425a-9455-6e13603277d5.png#clientId=u4cd2a735-5984-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=447&id=u86ed114a&margin=%5Bobject%20Object%5D&name=image.png&originHeight=894&originWidth=2152&originalType=binary&ratio=1&rotation=0&showTitle=false&size=284205&status=done&style=none&taskId=u34adb5c0-b42c-4135-baae-baf3da12e8e&title=&width=1076)
CProxy具体分为CProxyClient和CProxyServer，CProxyClient部署在与LocalServer同一个局域网内，CProxyServer部署到公网服务器上；
CProxyClient启动时，将需要进行数据转发的LocalServer注册到CProxyServer上，每注册一个LocalServer，CProxyServer就会多监听一个公网ip:port，这样，公网的PublicClient就能通过访问CProxyServer，最后将数据转发到内网的LocalServer上。

教程剩下来的章节，就是将上面的整个流程一步步实现。
## 项目规范
在扣具体实现细节前，我们先讲讲项目开发时的一些规范
### 项目基本目录结构
基本的目录结构其实在第三节就已经给出来了
```shell
├── client
│   ├── xxx.cpp
│   ├── ...
├── lib
│   ├── xxx.cpp
│   ├── ...
├── server
│   ├── xxx.cpp
│   ├── ...
├── include
│   ├── xxx.h
│   ├── ...
```
server目录是CProxy服务端目录，client目录是CProxy客户端目录，server和client分别能构建出可执行的程序；lib目录则存放一些被server和client调用的库函数；include目录则是存放一些第三方库。
### 引入第三方库 - spdlog日志库
spdlog是项目引入的一个日志库，也是唯一一个第三方库，主要是项目涉及到多线程，直接用print打日志调试实在是不方便；spdlog提供了比较丰富的日志格式，可以把日志时间戳、所在线程id、代码位置这些信息都打印出来。
#### 引入步骤

1. 项目spdlog代码仓库
```shell
git clone https://github.com/gabime/spdlog.git
```

2. 将spdlog/include/spdlog目录直接拷贝到CProxy项目的include目录下
2. 代码使用
```cpp
// 因为在CMakeLists中已经`include_directories(${PROJECT_SOURCE_DIR}/include)`,
// 表示在索引头文件时会查找到根目录下的include，
// 所以下面的写法最终会找到${PROJECT_SOURCE_DIR}/include/spdlog/spdlog.h
#include "spdlog/spdlog.h"
// 初始化日志格式
// format docs: https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
spdlog::set_pattern("[%@ %H:%M:%S:%e %z] [%^%L%$] [thread %t] %v");
// 打印日志
SPDLOG_INFO("cproxy server listen at: {}:{}", ip, port);
```
### 引入ccache加速编译
引入spdlog之后，可以发现每次编译都需要一首歌的时间，这开发调试时频繁编译，净听歌了。为了加速编译速度，项目引入了ccache，编译速度快过5G。ccache的原理和安装使用在第三节中有具体介绍，这里就不多废话了。
### 命名规则
原谅我这该死的代码洁癖，项目会规定一些命名规则，让代码读起来更优雅。。。，命名规则并没有什么标准，只要一个团队或一个项目内统一就行。
项目的命名规则大体是参考google的C++项目风格：[https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/)

- 文件名：全部小写，单词之间通过下划线连接，C++文件用.cpp结尾，头文件用.h结尾
- 类型/结构体：每个单词首字母均大写，如：MyExcitingClass
- 变量名：全部小写, 单词之间用下划线连接；类的私有成员变量以下划线结尾
- 函数名：常规函数名中每个单词首字母均大写，如：AddTableEntry；对于类的私有方法，首字母小写。
## 主要设计思路
### 业务概念设计
#### CProxyServer

- Control：在CProxyServer中会维护一个ControlMap，一个Control对应一个CProxyClient，存储CProxyClient的一些元信息和控制信息。
- Tunnel：每个Control中会维护一个TunnelMap，一个Tunnel对应一个LocalServer服务。
#### CProxyClient

- Tunnel：在CProxyClient端，也会维护一个TunnelMap，每个Tunnel对应一个LocalServer服务
> CProxyServer端的Tunnel与CProxyClient端的Tunnel存储的内容不一样，设计为两个不同的类。

### 连接设计
整个内网穿透需要的连接可分为两类：CProxyClient和CProxyServer之间传输元信息的控制连接、用于传输数据的数据连接

- 控制连接（Ctl_conn）

用于CProxyClient和CProxyServer之间各种事件的通知，比如CProxyClient向CProxyServer请求注册LocalServer、CProxyServer通知CProxyClient有新请求访问等

- 数据连接（tran_conn）

用于承载转发实际业务数据；业务数据传输会在LocalServer<->CProxyClient、CProxyClient<->CProxyServer和CProxyServer<->PublicClient三处地方进行。所以数据连接又细分为local_conn、proxy_conn和public_conn，方便进行不同的处理逻辑。
### 线程模型设计
我们主要采用多Reactor多线程模型配合IO多路复用实现高性能处理。
> 对Reactor线程模型和IO多路复用不熟的同学可以先回顾下第二节《day02 真正的高并发还得看IO多路复用》，再继续往下学习。

- **thread_pool**

thread_pool维护了一个工作线程列表，当reactor线程监听到有新连接建立时，可以从thread_pool中获取一个可用的工作线程，并由该工作线程处理新连接的读写事件，将每个连接的IO操作与reactor线程分离。

- **event_loop_thread**

event_loop_thread是reactor的线程实现，每个线程都有一个事件循环（One loop per thread），事件的监听及事件发生时的回调处理都是在这个线程中完成的。
thread_pool中的每个工作线程都是一个event_loop_thread, 主要负责连接套接字的read/write事件处理。
### 反应堆模式设计
这一部分讲的是event_loop_thread实现事件分发和事件回调的设计思路。

- **event_loop**

event_loop就是上面提到的event_loop_thread中的事件循环。简单来说，一个event_loop_thread被选中用来处理连接套接字fd时，fd会注册相关读写事件到该线程的event_loop上；当event_loop上注册的套接字都没有事件触发时，event_loop会阻塞线程，等待I/O事件发生。

- **event_dispatcher**

我们设计一个基类event_dispatcher，每个event_loop对象会绑定一个event_dispatcher对象，具体的事件分发逻辑都是由event_dispatcher提供，event_loop并不关心。
这是对事件分发的一种抽象。我们可以实现一个基于poll的poll_dispatcher，也可以是一个基于epoll的epoll_dispatcher。切换时event_loop并不需要做修改。

- **channel**

对各种注册到event_loop上的套接字fd对象，我们都封装成channel来表示。比如用于监听新连接的acceptor本身就是一个channel，用于交换元信息的控制连接ctl_conn和用于转发数据的tran_conn都有绑定一个channel用于存放套接字相关信息。
### 数据读写
- **buffer**

试想一下，有2kB的数据需要发送，但套接字发送缓冲区只有1kB。有两种做法：

**循环调用write写入**

在一个循环中不断进行write调用，等到系统将发送缓冲区的数据发送到对端后，缓冲区的空间就又能重新写入，这个时候可以把剩余的1kB数据写到发送缓冲区中。

这种做法的缺点很明显，我们并不知道系统什么时候会把发送缓冲区的数据发送到对端，这与当时的网络环境有关系。在循环过程中，线程无法处理其他套接字。

**基于事件回调**

在写入1kB之后，write返回，将剩余1kB数据存放到一个buffer对象中，并且监听套接字fd的可写事件（比如epoll的EPOLLOUT）。然后线程就可以去处理其他套接字了。等到fd的可写事件触发（代表当前fd的发送缓冲区有空闲空间），再调用write将buffer中的1kB数据写入缓冲区。这样可以明显提高线程的并发处理效率。

buffer屏蔽了套接字读写的细节。将数据写入buffer后，只要在合适的时机（可写事件触发时），告诉buffer往套接字写入数据即可，我们并不需要关心每次写了多少，还剩多少没写。

buffer设计时主要考虑尽量减少读写成本、避免频繁的内存扩缩容以及尽量减少扩缩容时的成本消耗。

## 总结
我们先是明确了项目的具体功能需求，然后提了开发过程中的一些规范，以便保持项目代码整洁。最后再是带着大家讲了下CProxy整个项目的设计思路。从下一节开始，就开始配合代码深入了解这些设计的具体实现。有能力的读者也可以先直接去看项目，读完这节后再看整个项目应该会清晰很多。

github地址：https://github.com/lzs123/CProxy，欢迎fork and star！

***如果本文对你有用，点个赞再走吧！或者关注我，我会带来更多优质的内容。***
![](https://files.mdnice.com/user/13956/11717db5-201b-4776-82e9-6f7eee7705db.png)



