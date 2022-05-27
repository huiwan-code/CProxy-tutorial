## 教程说明
[C++高性能网络服务保姆级教程](https://zhuanlan.zhihu.com/p/500269188)
## 首发地址
[day03 C++项目开发配置最佳实践(vscode远程开发配置、格式化、代码检查、cmake管理配置)](https://mp.weixin.qq.com/s?__biz=MzI4MTc0NDg2OQ==&amp;mid=2247484712&amp;idx=1&amp;sn=c6debd08b853be0fdaca3c2346b79952&amp;chksm=eba5c1a2dcd248b447a22b6498b98c4f4462532f54e096f643714b37706275a834cdf93bb8a0&token=1198088579&lang=zh_CN#rd)

经过前面两节课，我们已经从零开始写出了一个基于epoll和reactor多线程模型的高并发的网络服务器，为后面的内网穿透项目打下了相关基础。

工欲善其事，必先利其器，在继续往下讲项目的具体实现前，这节课带大家先把开发环境搭建配置好。

## vscode远程开发配置
由于后面项目用到了epoll和splice，这些都是unix环境中才有的调用，所以我们还是需要在linux环境中开发，我选用的vscode连接linux进行远程开发

### 选用vscode的原因
在这里只跟clion做下比较，clion支持的***full remote development***，基本原理是自动同步本地目录和远端服务器目录，在写代码时，用的还是本地环境，无法识别unix特有的那些头文件。

而vscode的远程开发，是把开发者自己机器上的 VSCode 原样拷贝到作为目标机器（Remote Host）上，以服务的形式运行，而本地的 VSCode 作为客户端，两者之间通过远程通讯协议彼此协调合作，实际上的开发工作主要是在服务端完成的。

### 配置流程
* [支持ssh公钥登录远程服务器](https://www.cnblogs.com/Hi-blog/p/9482418.html)
* 安装remote-ssh远程插件
![](https://files.mdnice.com/user/13956/53158b6d-a7f2-4f5b-9084-9c68736f4b79.png)


安装后重启可以在侧边栏看见这个
![](https://files.mdnice.com/user/13956/390d4ca5-c983-4920-9680-8c30a0852e58.png)


* 添加ssh target

点击「SSH TARGETS」旁边的「Configure」，选择编辑第一个文件（用户目录下的.ssh/config）
![](https://files.mdnice.com/user/13956/f2aed6e2-9d9e-4c35-a003-87d80d940f16.png)


在文件中填上服务器连接信息如下，更多配置信息可点击[这里](https://linux.die.net/man/5/ssh_config)

![](https://files.mdnice.com/user/13956/b2d649fb-88e2-4043-b49c-1461723b0da0.png)


* 添加远程工作区

点击「RemoteServer」后面的connection按钮，会打开一个新vscode窗口，等待连接远程服务器并完成一些初始化工作后，可点击「Open Folder」添加服务器的目录。

![](https://files.mdnice.com/user/13956/920f28e6-a7af-40c8-b357-428692744da3.png)


### 安装C++扩展
为了方便C++开发，我们需要添加C++扩展

![](https://files.mdnice.com/user/13956/1cafba69-a745-4470-b59e-4f4589284dea.png)


## clang-format格式化代码
开发一个项目时，一般是由多个程序员共同开发维护，如果每个人的编码习惯风格都不同，整个项目可能风格杂乱，可读性差，不利于项目维护。clang-format支持的代码风格有google、llvm、Chromium
Mozilla、WebKit，我们项目使用google风格。

### 安装clang-format
* ubuntu安装

直接从apt仓库安装即可
```shell
sudo apt-get install clang-format
```
* centos安装

centos 的yum仓库中并没有clang-format的安装包，需要更新repo源：
```shell
sudo yum install centos-release-scl-rh
```
之后下载clang-format：
```shell
sudo yum install llvm-toolset-7-git-clang-format
```
由于clang-format安装的位置不在系统的PATH变量中，所以这个时候在命令行还找不到`clang-format`命令。我们需要更新path变量，将clang-format的执行文件夹添加到path变量中：
1. 找到clang-format执行文件夹
```shell
sudo find / -name *clang-format*
```
```shell
...
/opt/rh/llvm-toolset-7/root/usr/bin/clang-format
...
```
2. 编辑`~/.bashrc`文件，更新path变量
```shell
export PATH=$PATH:/opt/rh/llvm-toolset-7/root/usr/bin
```

### 创建clang-format文件
输入以下命令就会按照google的格式在在当前路径下生成.clang-format文件。
```
clang-format -style=google -dump-config > .clang-format
```
大家只要讨论确认clang-format的具体内容，然后在项目根目录中加入这个文件，代码的风格问题就解决了。

### vscode支持clang-format
配置在vscode保存文件后自动进行格式化

***在扩展商店中搜索安装clang-format插件***
![](https://files.mdnice.com/user/13956/e4e0f842-2ef5-401d-9c0f-d74953b5fb7f.png)


***打开设置面板，之后在输入框输入clang-format，在「工作区」tab上找到style选项,修改为「file」，表示按照我们自己定义的.clang-format文件进行格式化***

![](https://files.mdnice.com/user/13956/e0893c54-3d3d-42b0-a4f1-13696f9f08ea.png)



***打开设置面板，在输入框中输入save，在「工作区」tab上把「format on save」选项勾选上***

![](https://files.mdnice.com/user/13956/d1961823-97c9-4a85-ae35-0c2c03565f76.png)



## 代码检查工具clang-tidy
clang-tidy是一个功能十分强大的代码检查工具，能帮助我们现代化代码，提高代码的可读性
### clang-tidy的安装
* ubuntu安装
```shell
sudo apt-get install clang-tidy
```

* centos安装
```shell
（1）sudo yum install centos-release-scl
（2）sudo yum install llvm-toolset-7
（3）sudo yum install llvm-toolset-7-clang-analyzer llvm-toolset-7-clang-tools-extra
（4）scl enable llvm-toolset-7 'clang -v'
（5）scl enable llvm-toolset-7 'lldb -v'
（6）scl enable llvm-toolset-7 bash
```

### clang-tidy使用
```shell
// 列出所有的check
$ clang-tidy -list-checks -checks='*'
// 找出simple.cc中所有没有用到的using declarations. 后面的`--`表示这个文件不在compilation database里面，可以直接单独编译；
$ clang-tidy -checks="-*,misc-unused-using-decls" path/to/simple.cc --

// 找出simple.cc中所有没有用到的using declarations并自动fix(删除掉)
$ clang-tidy -checks="-*,misc-unused-using-decls" -fix path/to/simple.cc --

// 找出a.c中没有用到的using declarations. 这里需要path/to/project/compile_commands.json存在
$ clang-tidy -checks="-*,misc-unused-using-decls" path/to/project/a.cc
```

如果在被分析的文件后面没有"--", clang-tidy会从目录下查找compliation database，这个database就是compile_commands.json文件，里面包含该项目中所有的编译单元的编译命令。
在使用之前要导出这个文件。目前已经有工具帮我们做了这项工作。
* 如果是cmake的项目，通过cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON path/to/llvm/sources命令导出;
> cmake版本需要大于3.5

* 如果是GYP项目，通过ninja -C out/D -t compdb cc cxx objc objcxx > compile_commands.json；
* 如果是make项目，使用[Bear](https://github.com/rizsotto/Bear)工具；

除了通过“-checks=”来设定检查规则，还可以在项目主目录之下添加.clang-tidy文件，在里面编写项目的检查规则，这种方式更加适合对整个项目进行定制化的规则编写。.clang-tidy文件并不是必须放在主目录之下，只是通常放在主目录之下方便对整个项目进行检查。
```shell
# .clang-tidy
Checks: '-*,clang-diagnostic-*,llvm-*,misc-*,-misc-unused-parameters,-misc-non-private-member-variables-in-classes,-readability-identifier-naming'
# Note that the readability-identifier-naming check is disabled, there are too
# many violations in the codebase and they create too much noise in clang-tidy
# results.
# Naming settings are kept for documentation purposes and allowing to run the
# check if the users would override this file, e.g. via a command-line arg.
CheckOptions:
  - key:             readability-identifier-naming.ClassCase
    value:           CamelCase
  - key:             readability-identifier-naming.EnumCase
    value:           CamelCase
  - key:             readability-identifier-naming.FunctionCase
    value:           camelBack
  - key:             readability-identifier-naming.MemberCase
    value:           CamelCase
  - key:             readability-identifier-naming.ParameterCase
    value:           CamelCase
  - key:             readability-identifier-naming.UnionCase
    value:           CamelCase
  - key:             readability-identifier-naming.VariableCase
    value:           CamelCase
```

上面的使用方法中，一次只能分析一个文件，如何一次性分析整个项目的文件呢？clang-tidy提供了[run_clang_tidy.py](https://github.com/llvm-mirror/clang-tools-extra/blob/master/clang-tidy/tool/run-clang-tidy.py)脚本，通过多进程的方法对整个项目文件进行分析。（具体使用方法可参考下面的cmake写法）


## cmake实现代码工程化
随着项目越来越复杂，模块越来越多，我们继续手动写makefile去构建项目显然不太合适，为了方便管理、构建复杂项目，使用cmake作为构建工具是个不错的选择。cmake是一个跨平台、开源的构建工具，可以方便的产生可移植的makefile，简化手动写makefile的工作量。

### 使用cmake生成makefile文件并编译一个分以下流程：
1. 在根目录及每个模块目录下编写CMakeLists.txt
2. 在根目录创建一个build文件夹
3. 进入build目录，执行cmake …/ 生成整个项目的makefile
4. 执行make和make install进行编译和安装。

> cmake的命令较多，详细教程可参考https://www.cnblogs.com/ybqjymy/p/13409050.html

### cmake实践
CProxy的代码目录结构如下
```shell
├── client
│   ├── xxx.cpp
│   ├── ...
├── lib
│   ├── xxx.cpp
│   ├── ...
├── server
│   ├── xxx.cpp
│   ├── ...
```
server目录是CProxy服务端目录，client目录是CProxy客户端目录，server和client分别能构建出可执行的程序；lib目录则存放一些被server和client调用的库函数。

首先，我们先在项目根目录上创建一个CMakeLists.txt
```shell
# cmake_minimum_required：指定了当前工程支持的cmake最小版本
cmake_minimum_required(VERSION 3.1)
# project：指定工程名称
project(CProxy)
# CMake 中有一个变量 CMAKE_BUILD_TYPE ,可以的取值是 Debug、Release、RelWithDebInfo和 MinSizeRel。
# 当这个变量值为 Debug 的时候,CMake 会使用变量 CMAKE_CXX_FLAGS_DEBUG 和 CMAKE_C_FLAGS_DEBUG 中的字符串作为编译选项生成 Makefile; 当变量值为Release时，则会使用CMAKE_CXX_FLAGS_RELEASE 和 CMAKE_C_FLAGS_RELEASE 中的字符串作为编译选项生成 Makefile。
SET(CMAKE_BUILD_TYPE "Debug")
# 启用GDB
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
# 启用优化（1～3）
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# 设置 c++ 编译器，这里使用clang++进行编译
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS -g -Wall)

message(STATUS "CMAKE_CXX_FLAGS: " "${CMAKE_CXX_FLAGS}")
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS: " "${CMAKE_CXX_FLAGS}")

# include_directories：将指定目录添加到编译器的头文件搜索路径之下，指定的目录被解释成当前源码路径的相对路径。
# 将工程根目录添加进来后，在server和client中能通过"lib/xxx"引入lib目录下的头文件
include_directories(${PROJECT_SOURCE_DIR})
# 添加子目录，并构建该子目录。
# 会执行lib、server、client三个目录中的CMakeLists.txt
add_subdirectory(lib)
add_subdirectory(server)
add_subdirectory(client)
```

上面的CMakeLists.txt添加了lib、server、client三个子目录，所以需要在这三个目录中也添加CMakeLists.txt
```shell
// lib/CMakeLists.txt
set(lib
    Buffer.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    Util.cpp
    EventLoop.cpp
    Channel.cpp
    Epoll.cpp
    Msg.cpp
    CtlConn.cpp
    ProxyConn.cpp
)

# 将${lib}变量指定的源文件生成链接文件
add_library(lib ${lib})
# target_link_libraries：将目标文件与库文件进行链接
# 使用多线程需要引入pthread库，所以将pthread库链接到上一步创建的lib目标文件中
target_link_libraries(lib pthread)
```

```shell
// client/CMakeLists.txt
# 将client目录下的所有源文件都存储到SOURCE_DIR变量中。 
aux_source_directory(./ SOURCE_DIR)
# 将${SOURCE_DIR}中的所有源文件编译成Client可执行文件
add_executable(Client ${SOURCE_DIR})
# 生成可执行文件需要链接lib库
target_link_libraries(Client lib)
```

```shell
// Server/CMakeLists.txt
aux_source_directory(./ SOURCE_DIR)
add_executable(Server ${SOURCE_DIR})
target_link_libraries(Server lib)
```

在根目录创建build目录，并执行```cmake ..``` 生成整个项目的makefile
```shell
mkdir build
cd build
cmake ..
``` 

在build目录下执行make进行编译
```
make
```

### clang-tidy在cmake中的配置
为了方便clang-tidy在项目中的使用，可以在根目录的CMakeLists.txt添加如下配置
```
# 用于输出clang-tidy需要用到的compile_commands.json文件
# 这一行需要放在add_subdirectory/aux_source_directory之前
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CLANG_SEARCH_PATH "/usr/local/bin" "/usr/bin" "/usr/local/opt/llvm/bin" "/usr/local/opt/llvm@8/bin" "/usr/local/Cellar/llvm/8.0.1/bin")
if (NOT DEFINED CLANG_TIDY_BIN)
    # attempt to find the binary if user did not specify
    find_program(CLANG_TIDY_BIN
            NAMES clang-tidy clang-fidy-8
            HINTS ${CLANG_SEARCH_PATH})
endif ()
if ("${CLANG_TIDY_BIN}" STREQUAL "CLANG_TIDY_BIN-NOTFOUND")
    message(WARNING "couldn't find clang-tidy.")
else ()
    message(STATUS "found clang-fidy at ${CLANG_TIDY_BIN}")
endif ()
# 添加clang-tidy命令
add_custom_target(clang-tidy COMMAND python ${CMAKE_SOURCE_DIR}/run-clang-tidy.py                              # run LLVM's clang-tidy script
-clang-tidy-binary ${CLANG_TIDY_BIN} # using our clang-tidy binary
-p ${CMAKE_BINARY_DIR}      # using cmake's generated compile commands
)
```

执行```cmake```获取到Makefile后，在build目录下执行```make clang-tidy```, 即可对整个项目进行代码分析。

### ccache加速编译
随着项目代码量越来越多，编译花费的时间会很长，在调试代码时，我们可能只改了一行代码，每次要编译个几分钟。这个时候就轮到ccache登场了。它将在第一遍编译时多花几秒钟，但接下来就会使编译成倍（5-10倍）的提速。

ccache 的基本原理是通过将头文件高速缓存到源文件之中而改进了构建性能，因而通过减少每一步编译时添加头文件所需要的时间而提高了构建速度。

#### ccache安装
```
yum install ccache
```

#### 结合cmake使用
在根目录的CMakeLists.txt加上下面这段代码
```
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache) # Less useful to do it for linking, see edit2
endif(CCACHE_FOUND)
```
重新进行编译，会发现第一遍还是比较久，但之后的编译速度就会变的很快了。

文章涉及到的代码文件可直接查看[CProxy](https://github.com/lzs123/CProxy),欢迎fork and star！


***如果本文对你有用，点个赞再走吧！或者关注我，我会带来更多优质的内容。***
![](https://files.mdnice.com/user/13956/11717db5-201b-4776-82e9-6f7eee7705db.png)
