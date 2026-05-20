---
marp: false
---
# 网络编程学习笔记

## 第1课：TCP/IP 基础概念

### 网络分层模型（四层简版）

```text
应用程序  ←→  TCP/UDP  ←→  IP  ←→  网线/网卡
(应用层)    (传输层)     (网络层)   (链路层)
```

消息从上往下**封装**，从下往上**解封装**。每层只关心自己的事。

### IP 地址 → 找机器

- IPv4：32 位，如 `192.168.1.1`
- `127.0.0.1` 指向本机（localhost）
- 类比：IP 是**小区地址**，帮你找到是哪栋楼

### 端口号 → 找进程

- 范围 0~65535
- 一台机器上一个 IP:端口 唯一标识一个进程
- HTTP 默认 80，我们的聊天室用 8888
- 客户端连接时系统自动分配临时端口
- 类比：IP 找到楼，**端口**找到房间里的具体人

### TCP 核心特性

| 特性     | 说明                                     |
| :------- | :--------------------------------------- |
| 面向连接 | 通信前先建立连接（三次握手）             |
| 可靠     | 丢包重传，乱序重排                       |
| 字节流   | **没有消息边界**——粘包问题的根源 |

对比：TCP 像**打电话**（先接通再说），UDP 像**对讲机**（按了就发不管对方）

### 三次握手（建立连接）

```text
Client → SYN          → Server  (我在吗？)
Client ← SYN+ACK      ← Server  (在，收到)
Client → ACK          → Server  (好，开始说)
```

- 由操作系统内核自动完成
- `connect()` 阻塞的时间就是在等三次握手完成

### 四次挥手（断开连接）

```text
Client → FIN           → Server  (我说完了)
Client ← ACK           ← Server  (收到)
Client ← FIN           ← Server  (我也说完了)
Client → ACK           → Server  (收到)
```

- `recv()` 返回 **0** 表示对端关闭了连接

---

## 第2课：socket 与阻塞 IO 模型

### socket 是什么

操作系统提供的一个**抽象接口**，让应用程序能通过网络收发数据。

```text
读文件：  open → read → write → close
网络通信：socket → connect/send/recv → closesocket
```

本质：操作 socket 和操作文件很像，数据经内核协议栈 → 网卡发出。

### TCP socket 类型

```cpp
SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
```

| 参数   | 值              | 含义               |
| :----- | :-------------- | :----------------- |
| 地址族 | `AF_INET`     | 使用 IPv4          |
| 类型   | `SOCK_STREAM` | 流式 socket（TCP） |
| 协议   | `IPPROTO_TCP` | TCP 协议           |

`SOCK_STREAM` = TCP，`SOCK_DGRAM` = UDP。

### 阻塞 IO 模型

**核心：函数调用不返回，直到条件满足。**

```text
服务端：  socket → bind → listen → accept（阻塞，等客户端连）
                                    ↓
                               recv（阻塞，等客户端发消息）
                                    ↓
                              收到数据，继续...
```

| 函数          | 阻塞原因       | 什么时候返回       |
| :------------ | :------------- | :----------------- |
| `accept()`  | 无客户端连接   | 有客户端连上来     |
| `connect()` | 三次握手未完成 | 连接成功或失败     |
| `recv()`    | 内核缓冲区为空 | 收到数据或对端关闭 |
| `send()`    | 内核缓冲区满了 | 缓冲区有空间了     |

**为什么阻塞是 chat server 的问题：**

单线程阻塞模型下，如果客户端 A 不说话，`recv(A)` 一直阻塞，B 和 C 都连不上。解决方案：**多线程**或 **I/O 多路复用（select/poll）**。

### 字节序

TCP/IP 协议规定网络传输用**大端（网络字节序）**，x86 CPU 用**小端**，所以需要转换：

| 函数        | 含义                               |
| :---------- | :--------------------------------- |
| `htons()` | Host TO Network Short（16 位端口） |
| `htonl()` | Host TO Network Long（32 位 IP）   |
| `ntohs()` | Network TO Host Short              |
| `ntohl()` | Network TO Host Long               |

---

## 第3课：echo server/client 实战

### 程序员需要记住的核心函数

#### 服务端生命周期

```text
WSAStartup → socket → bind → listen → accept → recv/send → closesocket → WSACleanup
```

| 函数                    | 必记 | 说明                                            |
| :---------------------- | :--- | :---------------------------------------------- |
| `WSAStartup()`        | ★   | Winsock 初始化，程序最开头调用一次              |
| `socket()`            | ★   | 创建 socket，返回 `SOCKET` 句柄               |
| `bind()`              | ★   | 绑定端口和 IP 到 socket                         |
| `listen()`            | ★   | 开始监听，第二个参数传 `SOMAXCONN`            |
| `accept()`            | ★   | 接受客户端连接，返回**新的**客户端 socket |
| `recv()` / `send()` | ★   | 收发数据                                        |
| `closesocket()`       | ★   | 关闭 socket                                     |
| `WSACleanup()`        | ★   | 清理 Winsock，程序最后调用一次                  |

#### 客户端生命周期

```text
WSAStartup → socket → connect → send/recv → closesocket → WSACleanup
```

| 函数          | 必记 | 说明                             |
| :------------ | :--- | :------------------------------- |
| `connect()` | ★   | 连接服务器，传入服务器地址和端口 |

#### 辅助函数（理解含义即可）

| 函数                  | 说明                                                |
| :-------------------- | :-------------------------------------------------- |
| `htons()`           | 端口转网络字节序（记住这个就行）                    |
| `inet_addr()`       | 字符串 IP → 二进制，如 `"127.0.0.1"`             |
| `inet_ntoa()`       | 二进制 IP → 字符串（只在服务端打印客户端 IP 时用） |
| `WSAGetLastError()` | 获取错误码，配合 `std::cerr` 打印                 |

### 服务端框架（填空式模板）

```cpp
// 1. 初始化
WSADATA wsaData;
if (WSAStartup(MAKEWORD(2,2), &wsaData)) { return -1; }

// 2. 创建监听 socket
SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
if (listenSock == INVALID_SOCKET) { WSACleanup(); return -1; }

// 3. bind
sockaddr_in serverAddr{};
serverAddr.sin_family = AF_INET;
serverAddr.sin_addr.s_addr = INADDR_ANY;
serverAddr.sin_port = htons(PORT);
if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
    closesocket(listenSock); WSACleanup(); return -1;
}

// 4. listen
if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
    closesocket(listenSock); WSACleanup(); return -1;
}

// 5. accept
sockaddr_in clientAddr{};
int addrLen = sizeof(clientAddr);
SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);

// 6. 通信循环
char buf[1024];
int bytesReceived;
while (true) {
    bytesReceived = recv(clientSock, buf, 1024, 0);
    if (bytesReceived > 0) {
        send(clientSock, buf, bytesReceived, 0);
    } else {
        break;  // 0 = 断开, <0 = 出错
    }
}

// 7. 清理
closesocket(clientSock);
closesocket(listenSock);
WSACleanup();
```

### 客户端框架（填空式模板）

```cpp
// 1. 初始化
WSADATA wsaData;
if (WSAStartup(MAKEWORD(2,2), &wsaData)) { return -1; }

// 2. 创建 socket
SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

// 3. connect
sockaddr_in serverAddr{};
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(PORT);
serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));

// 4. 通信循环
char buf[1024];
string input;
while (true) {
    getline(cin, input);
    send(sock, input.c_str(), input.size(), 0);
    int n = recv(sock, buf, 1024, 0);
    string echo(buf, n);
    cout << echo << endl;
}

// 5. 清理
closesocket(sock);
WSACleanup();
```

### 关键理解

- `accept()` 返回的 socket 和监听 socket 是**两个不同的东西**：监听 socket 只负责接客，client socket 才负责聊天
- `recv()` 返回 **0** 是判断客户端断开的唯一标准
- `send()` 第三个参数传**实际字节数**（`bytesReceived`），不是缓冲区大小（`BUFFER_SIZE`），否则会多发垃圾数据
- `char[]` 不保证末尾有 `\0`，打印用 `string(buf, n)` 构造指定长度

---

## 第4课：I/O 多路复用与 select

### 为什么需要 select

阻塞模型一个线程只能处理一个客户端，A 没发消息时 `recv(A)` 卡住，B 连不上。select 让一个线程监视多个 socket。

### select 核心

```cpp
select(nfds, &readfds, &writefds, &exceptfds, timeout);
```

实践中只用 `readfds`——监视哪些 socket 有数据可读。

| 参数        | 作用                                      |
| :---------- | :---------------------------------------- |
| `readfds` | 要监视"可读"的 socket 集合                |
| `timeout` | `NULL` = 无限等待，`{0,0}` = 立即返回 |

### fd_set 操作宏（必须记住）

| 宏                    | 作用                           |
| :-------------------- | :----------------------------- |
| `FD_ZERO(&set)`     | 清空集合                       |
| `FD_SET(s, &set)`   | 把 socket s 加入集合           |
| `FD_ISSET(s, &set)` | select 返回后检查 s 是否有事件 |
| `FD_CLR(s, &set)`   | 从集合移除 s                   |

### select 典型模式

```text
while (true) {
    FD_ZERO(&readSet);
    FD_SET(监听socket, &readSet);   // 监视新连接
    FD_SET(客户端A, &readSet);       // 监视 A 的消息
    FD_SET(客户端B, &readSet);       // 监视 B 的消息
    ...

    select(..., &readSet, ...);      // 阻塞等待

    if (FD_ISSET(监听socket, &readSet))
        接受新连接，加入 clients 列表

    for (遍历所有客户端) {
        if (FD_ISSET(clients[i], &readSet))
            recv() + 处理
    }
}
```

### 服务端模板（select 版）

```cpp
#include <vector>
#include <winsock2.h>
// ... 初始化、bind、listen 同上 ...

std::vector<SOCKET> clients;

while (true) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(listenSock, &readSet);
    int maxFd = (int)listenSock;

    for (SOCKET s : clients) {
        FD_SET(s, &readSet);
        if (s > maxFd) maxFd = (int)s;
    }

    int ret = select(maxFd + 1, &readSet, NULL, NULL, NULL);
    if (ret == SOCKET_ERROR) break;

    // 有新连接
    if (FD_ISSET(listenSock, &readSet)) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
        if (clientSock != INVALID_SOCKET) {
            clients.push_back(clientSock);
            std::cout << "Client connected" << std::endl;
        }
    }

    // 有消息
    for (int i = 0; i < (int)clients.size(); i++) {
        if (FD_ISSET(clients[i], &readSet)) {
            char buf[1024];
            int n = recv(clients[i], buf, 1024, 0);
            if (n > 0) {
                for (SOCKET s : clients)
                    if (s != clients[i]) send(s, buf, n, 0);
            } else {
                closesocket(clients[i]);
                clients.erase(clients.begin() + i);
                i--;
            }
        }
    }
}
```

### 关键理解

- `select()` 的 `readfds` 是**输入也是输出**：传入你要监视的集合，返回时集合里只保留有事件的 socket
- 监听 socket 也放在 `readfds` 里，有新连接时它变成"可读"
- 客户端断开时 `recv()` 返回 0，记得从 `clients` 列表移除并 `closesocket()`
- `FD_SETSIZE` 默认上限 64，select 适合几十个连接，更多连接用 IOCP（Windows）或 epoll（Linux）

---

## 第5课：自定义协议与粘包处理

### 为什么 TCP 会粘包

TCP 是流协议，没有消息边界：

```text
发送方：send("hello") → send("world")
接收方：recv() 可能一次收到 "helloworld"（粘包）
       recv() 也可能收到 "hel" + "loworld"（半包）
```

### 解决方案：长度前缀协议

每条消息前加 4 字节头部记录长度：

```text
┌──────────────┬──────────────────────────┐
│ 长度 (4B)    │ 消息内容 (N 字节)         │
│ htonl(N)     │ data[0..N-1]             │
└──────────────┴──────────────────────────┘
```

### 三个辅助函数（记入模板）

```cpp
// 保证收满 len 字节（处理半包）
int recv_all(SOCKET s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
}

// 按协议收一条完整消息：先读4字节长度，再读内容
int recv_msg(SOCKET s, char* buf, int bufSize) {
    int netLen;
    if (recv_all(s, (char*)&netLen, 4) <= 0) return -1;
    int len = ntohl(netLen);
    if (len > bufSize) return -1;
    if (recv_all(s, buf, len) <= 0) return -1;
    return len;
}

// 按协议发一条消息：先发4字节长度，再发内容
void send_msg(SOCKET s, const char* data, int len) {
    int netLen = htonl(len);
    send(s, (char*)&netLen, 4, 0);
    send(s, data, len, 0);
}
```

### 登录协议

客户端连接后先发 `/name:xxx` 注册用户名：

```text
客户端 → 服务端：/name:alice
服务端：存下名字 "alice"，不广播这条消息
```

服务端判断逻辑：

```cpp
// 名字为空且消息以 /name: 开头 → 登录消息
if (client.name.empty() && msg.rfind("/name:", 0) == 0) {
    client.name = msg.substr(6);
    continue;  // 不广播
}
```

### 退出通知

客户端断开时（`recv_msg` 返回 `-1`），服务端广播 `"xxx left"`。

### 项目文件结构

```text
webServer/
├── CMakeLists.txt     ← CMake 构建
├── build.bat          ← 一键编译
├── run_server.bat     ← 启动服务端
├── run_client.bat     ← 启动客户端
├── .gitignore
├── learning.md
└── src/
    ├── echo_server.cpp
    └── echo_client.cpp
```

### 当前功能清单

- [X] echo 通信（一对一回显）
- [X] 多客户端广播（select）
- [X] 自定义协议（长度前缀）
- [X] 用户名登录
- [X] 退出通知

### 协议安全问题

当前协议 `[长度4B][消息N]` 有一个致命漏洞：**没有合法性校验**。任何数据的头 4 字节都会被当成长度字段解析。如果有人用 telnet 连上来随便打字，头 4 字节可能解析出负数、超大数，导致越界或逻辑错误。

**解决：加魔数（Magic Number）**

```text
旧协议： [长度 4B][消息 N 字节]
新协议： [魔数 0xAB 0xCD][长度 2B][消息 N 字节]
                  ↑
          固定两个字节，作为"身份证"
```

**校验流程：**

```text
收到数据 → 前 2 字节 == 0xABCD？
  → 是  → 读长度 → 读消息体 → 正常处理
  → 否  → 非法数据，立即断开连接
```

魔数就像暗号——对不上就踢掉。常见的魔数：PNG 文件头 `89 50 4E 47`，JVM class 文件头 `CA FE BA BE`。

### 面试话术

> "我的协议在长度前缀前加了 2 字节魔数做合法性校验。收到数据时先比对魔数，不匹配就认为是非法连接直接断开。这跟 PNG 文件头、HTTP 的 `\r\n\r\n` 分隔符是一个思路——在字节流中加标志位来识别和验证数据。"

---

## 第6课：epoll

### select 的问题

| 问题      | 说明                                                    |
| :-------- | :------------------------------------------------------ |
| O(n) 扫描 | 每次 `select()` 返回后要遍历所有 fd 检查 `FD_ISSET` |
| fd 上限   | `FD_SETSIZE` 默认 1024（改大也影响性能）              |
| 数据拷贝  | 每次调用都要把 fd_set 从用户态拷进内核再拷出来          |

### epoll 怎么解决的

epoll 在内核里维护一个**红黑树**存储所有注册的 fd，用**就绪链表**直接返回有事件的 fd，做到 O(1)。

```text
select：  用户态: [fd1,fd2,fd3]  →  内核: 扫描全部  →  返回: 哪些有事件（需遍历）
epoll：   内核维护红黑树 + 就绪链表，只返回有事件的 fd（不需遍历）
```

### 三个核心 API

```cpp
#include <sys/epoll.h>

// 1. 创建 epoll 实例，返回一个 fd
int epfd = epoll_create1(0);

// 2. 添加/修改/删除 要监听的 fd
struct epoll_event ev;
ev.events = EPOLLIN;           // 监视可读事件
ev.data.fd = socket_fd;        // 用户数据（传 fd 进去）
epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &ev);   // 添加
epoll_ctl(epfd, EPOLL_CTL_MOD, socket_fd, &ev);   // 修改
epoll_ctl(epfd, EPOLL_CTL_DEL, socket_fd, NULL);  // 删除

// 3. 等待事件（类似 select 的阻塞等待）
struct epoll_event events[MAX_EVENTS];
int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
// events[0..n-1] 里就是所有就绪的 fd，不需遍历所有客户端！
```

### API 对照表

| 操作     | select 做法                        | epoll 做法                                     |
| :------- | :--------------------------------- | :--------------------------------------------- |
| 初始化   | `fd_set` + `FD_ZERO`           | `epoll_create1(0)`                           |
| 添加监听 | `FD_SET(sock, &readSet)`         | `epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev)`  |
| 等待事件 | `select(maxFd+1, &readSet, ...)` | `epoll_wait(epfd, events, MAX, -1)`          |
| 检查事件 | `FD_ISSET(sock, &readSet)`       | 遍历 `events[0..n]` 即是有事件的             |
| 移除监听 | `FD_CLR` + 不加入                | `epoll_ctl(epfd, EPOLL_CTL_DEL, sock, NULL)` |

### 两个触发模式

| 模式          | 常量        | 行为                           | 适用                        |
| :------------ | :---------- | :----------------------------- | :-------------------------- |
| 水平触发 (LT) | 默认        | 只要缓冲区有数据就通知         | 简单，select 用户直接上手   |
| 边缘触发 (ET) | `EPOLLET` | 只在"无→有"的那一瞬间通知一次 | 必须配合非阻塞 IO，性能更好 |

**本次用 LT（默认）就够**，行为跟 select 一致。

### epoll 版主循环模板

```cpp
int epfd = epoll_create1(0);

// 注册监听 socket
struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = listenSock;
epoll_ctl(epfd, EPOLL_CTL_ADD, listenSock, &ev);

struct epoll_event events[64]; // 一次最多返回 64 个就绪事件

while (true) {
    int n = epoll_wait(epfd, events, 64, -1);  // 阻塞等待
    if (n < 0) break;

    for (int i = 0; i < n; i++) {
        int fd = events[i].data.fd;

        if (fd == listenSock) {
            // 新连接
            int clientSock = accept(listenSock, ...);
            ev.events = EPOLLIN;
            ev.data.fd = clientSock;
            epoll_ctl(epfd, EPOLL_CTL_ADD, clientSock, &ev);
        } else {
            // 客户端消息
            int bytes = recv_msg(fd, buf, BUFFER_SIZE);
            if (bytes > 0) {
                // 广播给其他客户端
            } else {
                // 断开：从 epoll 移除 + close
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
            }
        }
    }
}
```

### 关键理解

- epoll 的 `events[0..n-1]` **只包含就绪的 fd**，不需要像 select 那样遍历所有客户端
- 客户端断开时**必须** `epoll_ctl(EPOLL_CTL_DEL)` 从 epoll 中移除，否则内核会继续监视一个已关闭的 fd
- `data.fd` 只是 `epoll_event` 的一个字段，你可以用它存 fd 也可以存指针（存指针在高级用法里很有用）
- 默认就是 LT（水平触发），行为跟 select 一样，不用改 `recv_all` 等函数

---

## 第7课：非阻塞 IO 与 epoll ET 边缘触发

### 阻塞 vs 非阻塞

```text
阻塞 recv(fd, buf, 1024, 0):
  内核缓冲区空 → 线程挂起等待 → 有数据才返回

非阻塞 recv(fd, buf, 1024, 0):
  内核缓冲区空 → 立即返回 -1，errno = EAGAIN/EWOULDBLOCK
  有数据 → 正常返回
```

**非阻塞模式只有一个操作：**

```cpp
#include <fcntl.h>

int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

或者创建 socket 时一步到位：

```cpp
int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
```

### 为什么要非阻塞

在 **epoll ET 模式**下，`epoll_wait` 只在 fd 状态变化时通知一次：

```text
LT（水平触发）：         ET（边缘触发）：
缓冲区有数据 → 通知     缓冲区 空→有数据 → 通知一次
                        你不读完？不管了，不再通知
调用者仍在读 → 继续通知
```

**ET 下必须循环读到 EAGAIN，否则数据就烂在内核缓冲区里：**

```cpp
// ET 模式的正确读法
while (true) {
    int n = recv(fd, buf, 1024, 0);
    if (n > 0)  { 处理数据; continue; }
    if (n == 0) { 对端关闭; break; }
    if (n < 0 && errno == EAGAIN) break;  // 读完了，正常
    if (n < 0 && errno != EAGAIN) { 出错; break; }
}
```

### ET 下的 accept 也要循环

高并发时多个连接同时到达，`listenSock` 只通知一次：

```cpp
while (true) {
    int clientSock = accept4(listenSock, ..., SOCK_NONBLOCK);
    if (clientSock >= 0) { 处理新连接; }
    else if (errno == EAGAIN) break;
    else { 出错; break; }
}
```

### LT vs ET 对比

|           | LT（水平触发）         | ET（边缘触发）            |
| :-------- | :--------------------- | :------------------------ |
| 通知策略  | 只要缓冲区有数据就通知 | 只在状态变化时通知一次    |
| recv 调用 | 可以只读一部分         | **必须读到 EAGAIN** |
| 使用难度  | 简单                   | 必须非阻塞 + 循环读       |
| 适用场景  | 简单服务、兼容 select  | 高并发、低延迟            |
| 触发次数  | 更多（可能重复通知）   | 更少（不会有冗余通知）    |

### 本次改造目标

把 `echo_server.cpp` 改成 **epoll ET + 非阻塞 IO**：

1. `socket()` / `accept4()` 创建时直接非阻塞
2. `epoll_ctl` 注册时加 `EPOLLET`
3. `recv_all` 改成循环读到 EAGAIN
4. accept 也改成循环

---

## 第8课：Reactor 模式

### 什么是 Reactor

Reactor = EventLoop + Channel。把 select/epoll 的裸循环封装成可复用的类。

**模型：**

```text
            ┌────────────────────────┐
            │     EventLoop          │
            │  (epoll_wait 循环)      │
            │                        │
            │  Channel{listenSock} ──→ handleEvent: accept
            │  Channel{client1}    ──→ handleEvent: recv + broadcast
            │  Channel{client2}    ──→ handleEvent: recv + broadcast
            │  Channel{timerfd}    ──→ handleEvent: 定时回调
            └────────────────────────┘
```

不是只能在 while(1) 里写业务逻辑，而是每个 fd 绑定一个回调函数。

### 两个核心类

**Channel：一个 fd + 它的回调**

```cpp
class Channel {
    int fd_;
    uint32_t events_;    // 关心这个 fd 的哪些事件
    std::function<void()> readCallback_;
    std::function<void()> writeCallback_;
    std::function<void()> closeCallback_;

    void handleEvent(uint32_t revents) {
        if (revents & EPOLLIN)  readCallback_();
        if (revents & EPOLLOUT) writeCallback_();
        if (revents & (EPOLLHUP | EPOLLERR)) closeCallback_();
    }
};
```

**EventLoop：驱动引擎**

```cpp
class EventLoop {
    int epfd_;
    std::vector<Channel*> channels_;

    void loop() {
        while (true) {
            int n = epoll_wait(epfd_, events, 64, -1);
            for (int i = 0; i < n; i++) {
                Channel* ch = (Channel*)events[i].data.ptr;
                ch->handleEvent(events[i].events);
            }
        }
    }

    void updateChannel(Channel* ch) {
        epoll_ctl(epfd_, EPOLL_CTL_ADD/MOD/DEL, ch->fd(), &ev);
    }
};
```

注意 `data.ptr` 存的是 `Channel*` 指针，不再存 `fd`。

### 为什么面试必问

- Reactor 是 Nginx、Redis、Netty、muduo 的基石模式
- 把 "I/O 事件分发" 和 "业务逻辑" 解耦
- 面试常手写简化版 Reactor

### 本次改造目标

把 `echo_server.cpp` 拆成 `Channel` + `EventLoop` 两个类，主循环只调 `loop.loop()`。

### 实战：重构后代码结构

**之前（裸 epoll）：**

```text
┌──────────────────────────────────┐
│ while(running) {                 │
│     epoll_wait(...);             │
│     for(i < n) {                 │
│         if(fd == STDIN)    {...} │  ← 终端命令
│         else if(fd == listen) {} │  ← 新连接
│         else { 30行业务逻辑 }    │  ← 客户端消息
│     }                            │
│ }                                │
└──────────────────────────────────┘
所有逻辑堆在 main() 的 while/if/else 里
```

**之后（Reactor）：**

```text
┌────────────────────┐
│ EventLoop loop;    │
│                    │
│ Channel(stdin)     │  .onRead(→quit)
│ Channel(listen)    │  .onRead(→accept循环)
│ Channel(client1)   │  .onRead(→读缓冲→parse→广播)
│ Channel(client2)   │  .onRead(→读缓冲→parse→广播)
│                    │
│ loop.loop();       │  ← 一行启动
└────────────────────┘
```

### 实战：实际实现的类

**Channel 类：**

```cpp
class Channel {
public:
    Channel(EventLoop* loop, int fd);

    void setReadCallback(std::function<void()> cb);
    void enableReading(bool et = true);   // true=ET, false=LT
    void disableAll();
    void handleEvent(uint32_t revents);
    int fd() const;

private:
    EventLoop* loop_;
    int fd_;
    uint32_t events_ = 0;
    std::function<void()> readCb_;
    std::function<void()> closeCb_;
    bool added_ = false;                 // 是否已注册到 epoll

    friend class EventLoop;              // EventLoop 需要访问 events()、added()
};
```

**EventLoop 类：**

```cpp
class EventLoop {
public:
    EventLoop();                          // epoll_create1
    ~EventLoop();                         // close(epfd)

    void updateChannel(Channel* ch);      // epoll_ctl(ADD/MOD)
    void removeChannel(Channel* ch);      // epoll_ctl(DEL) + setAdded(false)
    void loop();                          // while(running) epoll_wait → handleEvent
    void quit();                          // running_ = false

private:
    int epfd_;
    bool running_ = true;
};
```

### 实战关键细节

| 细节                                | 说明                                                                                                 |
| :---------------------------------- | :--------------------------------------------------------------------------------------------------- |
| `ev.data.ptr = channel`           | epoll 事件返回时直接拿到 `Channel*`，不再存 fd 再查下标                                            |
| `enableReading(bool et)`          | `true` 设 `EPOLLIN\|EPOLLET`，`false` 只设 `EPOLLIN`                                          |
| `friend class EventLoop`          | EventLoop 的 `updateChannel` 需要读 Channel 的 `events()` `added()`                            |
| `removeChannel` vs `disableAll` | `disableAll()` 调用 `removeChannel()`，后者只做 syscall + 改标志，**不能互相调用导致递归** |
| lambda 捕获 `[&, clientChannel]`  | 按值捕获指针，其余按引用                                                                             |

### 核心收获

- **解耦**：I/O 事件分发（EventLoop）和业务逻辑（lambda 回调）完全分离
- **可扩展**：加 stdin 退出一项 = 加一个 Channel，主循环不用改
- **可复用**：EventLoop + Channel 可以直接拿到下个项目用
- `data.ptr` 存指针是 Reactor 模式的关键，避免了 select 时代的下标查找
- 每个 fd = 一个 Channel = 一套回调，if/else 分支被 lambda 替代

---

## 第9课：线程池

### 餐厅类比

```text
你现在的单线程 Reactor = 一个服务员：

  服务员：领位 → 点菜 → 去厨房炒菜 → 端菜
                         ↑
              炒菜 5 分钟，其他客人全晾着

线程池 = 雇一群厨师，服务员只传菜：

  服务员：领位 → 点菜 → 撕菜单扔后厨 → 回去服务下一位
  厨师们：等菜单 → 做菜 → 叫服务员端出去
```

### 代码对应

```text
EventLoop 线程                      工作线程（厨师）
──────────                        ─────────
epoll_wait()                      while(true) {
→ recv 收到消息                     从队列取任务
→ pool.addTask([处理消息]){         执行任务
      // 扔进去就返回                 }
  }
→ 继续 epoll_wait() 等下一个
```

**`addTask` 立即返回，不阻塞 epoll_wait。**

### 核心实现：一个队列 + 一把锁

```cpp
// 共享数据
std::queue<function> taskQueue;  // 任务池
std::mutex        mtx;           // 保证同时只有一人操作队列
std::condition_variable cv;      // 厨师睡觉/叫醒

// EventLoop：生产者（放任务）
void addTask(task) {
    lock(mtx);
    taskQueue.push(task);
    unlock(mtx);
    cv.notify_one();    // 叫醒一个厨师
}

// 工作线程：消费者（取任务）
void workerLoop() {
    while(true) {
        lock(mtx);
        if (队列空) { unlock(mtx); cv.wait(); continue; }  // 睡觉
        取走任务;
        unlock(mtx);
        执行任务();        // ← 不持锁执行，不阻塞其他人
    }
}
```

### 三个核心概念

| 概念                   | 作用                         | 一句话                       |
| :--------------------- | :--------------------------- | :--------------------------- |
| `mutex`              | 保证同时只有一个人操作队列   | 共享数据要排队               |
| `condition_variable` | 空队列时厨师睡觉，有活了叫醒 | 不空转 CPU                   |
| 不持锁执行             | 任务执行在 `unlock` 之后   | 一个厨师做菜不影响别人取任务 |

### 聊天室现在需要吗？

**不需要。** 广播就是 `send()`——很快。但面试时可以说：如果消息涉及数据库查询、加密等耗时操作，就扔进线程池处理。

### 典型面试话术

> "Reactor + 线程池：EventLoop 负责 IO 快进快出，线程池处理耗时计算，两者通过任务队列解耦。"

---

## 第10课：定时器

### 为什么需要定时器

| 场景      | 说明                                               |
| :-------- | :------------------------------------------------- |
| 心跳/踢线 | 客户端 30 秒不发消息 → 断开（防止死连接占用资源） |
| 定期统计  | 每 10 秒打印在线人数                               |

### 方式一：timerfd（本次用这个）

timerfd 就是**一个特殊的 fd**，时间到了就变成"可读"，直接注册到 epoll 跟 socket 一样处理。

```text
普通 socket：  对端发数据 → fd 变成可读 → epoll 通知
timerfd：      时间到了   → fd 变成可读 → epoll 通知
```

完全不需要改 EventLoop、不需要单独搞定时器线程。

**三步使用：**

```cpp
#include <sys/timerfd.h>

// 1. 创建（非阻塞）
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

// 2. 设置：首次 10 秒后触发，之后每 10 秒一次
struct itimerspec ts{};
ts.it_value.tv_sec = 10;      // 首次到期时间
ts.it_interval.tv_sec = 10;   // 重复间隔（=0 表示只触发一次）
timerfd_settime(tfd, 0, &ts, nullptr);

// 3. 当普通 fd 注册到 epoll
Channel timerChan(&loop, tfd);
timerChan.enableReading(false);   // LT 模式
timerChan.setReadCallback([&]() {
    // 时间到了，消费掉事件（read 一次，否则 LT 会反复通知）
    uint64_t exp;
    read(tfd, &exp, sizeof(exp));   // 读出到期次数

    std::cout << "[INFO] 当前在线: " << clients.size() << " 人" << std::endl;
});
```

**必须 `read(tfd, ...)`** 消费事件，否则 LT 模式下 epoll 会一直通知。

### `itimerspec` 字段解释

```cpp
struct itimerspec {
    struct timespec it_value;    // 首次到期时间
    struct timespec it_interval; // 重复间隔
    // 每个 timespec: { tv_sec (秒), tv_nsec (纳秒) }
};

// 例子：
ts.it_value.tv_sec = 10;       // 10 秒后首次触发
ts.it_interval.tv_sec = 30;    // 之后每 30 秒触发一次

// 只触发一次：
ts.it_value.tv_sec = 5;
ts.it_interval.tv_sec = 0;     // ← 0 表示不重复
```

### 心跳踢线怎么做

基本思路：每个客户端记录 `lastActiveTime_`，定时器每 10 秒扫描一次：

```text
定时器回调（每 10 秒）：
  for (遍历 clients)：
    if (now - client.lastActiveTime > 30s)：
      断开这个客户端
```

每次收到消息时更新 `lastActiveTime_`，就相当于把任何消息（包括聊天消息）都当心跳。比单独发 `/ping` 更简单。

### 方式二：定时器队列（跨平台，不展开）

不用 timerfd，而是维护一个按过期时间排序的堆，`epoll_wait` 传入最近定时器的剩余时间作为超时参数。timerfd 更简单，推荐新手。

### 本次改造目标

用 timerfd 给聊天室加上：

1. **定期统计**：每 10 秒打印当前在线人数
2. **心跳踢线**：客户端 30 秒没发消息就踢掉

改动量很小——加一个 Channel，不碰已有逻辑。

---

## 第11课：HTTP/1.1 协议

### HTTP 请求格式

```text
GET /index.html HTTP/1.1\r\n        ← 请求行
Host: example.com\r\n                ← 头部（Host 必带）
Connection: keep-alive\r\n
\r\n                                  ← 空行，头部结束
                                     ← 以下为 Body（GET 无 Body）
```

```
POST /api/login HTTP/1.1\r\n
Host: example.com\r\n
Content-Type: application/json\r\n
Content-Length: 27\r\n               ← Body 的字节数
\r\n
{"username":"a","pass":"b"}           ← Body
```

### HTTP 响应格式

```text
HTTP/1.1 200 OK\r\n                  ← 状态行
Content-Type: text/html\r\n
Content-Length: 13\r\n
\r\n
<html>hello</html>                   ← Body
```

### 关键字段

| 字段                           | 说明                                               |
| :----------------------------- | :------------------------------------------------- |
| `Content-Length`             | Body 的字节数，解决粘包（类比长度前缀）            |
| `Connection: keep-alive`     | 一个 TCP 连接可以发多个 HTTP 请求（HTTP/1.1 默认） |
| `Transfer-Encoding: chunked` | 另一种粘包方案：分块传输                           |

### 最小 HTTP 服务端流程

```cpp
// 1. 收到数据
// 2. 找到 \r\n\r\n 分隔头部和 Body
// 3. 解析第一行 → 方法、路径
// 4. 如果有 Content-Length，按长度读 Body
// 5. 构造响应：状态行 + Content-Length + \r\n\r\n + Body
// 6. send 回去
```

### 实战：给聊天室加 HTTP 接口

**方案：TCP 连接后根据首条消息判断协议**

```text
首条消息:
  以 "GET " 或 "POST " 开头 → HTTP 客户端 → 返回网页 → 断开
  以 "/name:" 开头          → 聊天客户端 → 正常协议 → 保持连接
```

**实现：**

```cpp
// ClientInfo 移到全局，handle_http 才能访问
struct ClientInfo {
    int sock;
    std::string name;          // 名字为空 = 未登录
    std::string inBuf;
    Channel* channel;
    time_t lastActiveTime;
};

bool handle_http(int fd, const std::string& inBuf, std::vector<ClientInfo>& clients) {
    if (inBuf.rfind("GET ", 0) != 0) return false;  // 不是 HTTP

    // 解析路径："GET / HTTP/1.1..." → "/"
    size_t start = inBuf.find(' ') + 1;
    size_t end   = inBuf.find(' ', start);
    std::string path = inBuf.substr(start, end - start);

    std::string body, status;
    if (path == "/") {
        status = "200 OK";
        // 只统计已有名字的客户端（排除 HTTP 连接自己）
        int online = 0;
        for (auto& c : clients) if (!c.name.empty()) online++;
        body = "<html>...在线用户 (" + std::to_string(online) + ")</html>";
    } else {
        status = "404 Not Found";
        body   = "404 Not Found";
    }

    std::string resp = "HTTP/1.1 " + status + "\r\n"
                       "Content-Type: text/html; charset=utf-8\r\n"
                       "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    send(fd, resp.c_str(), resp.size(), 0);
    return true;
}
```

**调用位置：** 在客户端消息回调中，`read_to_buf` 之后、`parse_msg` 之前：

```cpp
// 读数据后先检查是否是 HTTP
if (handle_http(clients[idx].sock, clients[idx].inBuf, clients)) {
    // HTTP 已处理，断开连接
    clientChannel->disableAll();
    close(clients[idx].sock);
    delete clientChannel;
    clients.erase(clients.begin() + idx);
    return;
}
// 不是 HTTP，走正常聊天协议
parse_msg(...)
```

### 核心收获

- HTTP 是纯文本协议，头部与 Body 用 `\r\n\r\n` 分隔
- 首条消息判断协议类型即可让同一端口同时服务浏览器和聊天客户端
- 在线人数统计要排除 HTTP 连接（没名字的），否则会把浏览器自己也数进去
- Content-Length 必须精确，字节数 = 字符串的 `.size()`

---

## 第12课：高性能设计总结

### 五种 I/O 模型（面试高频）

| 模型         | 特点                 | 代表函数                      |
| :----------- | :------------------- | :---------------------------- |
| 阻塞 I/O     | 线程挂起等数据       | `recv()` 默认               |
| 非阻塞 I/O   | 没数据立即返回       | `fcntl` + `recv`          |
| I/O 多路复用 | 一个线程等多个 fd    | `select`/`poll`/`epoll` |
| 信号驱动 I/O | 内核发信号通知       | `SIGIO`（少用）             |
| 异步 I/O     | 内核完成所有操作回调 | `aio_read` / `io_uring`   |

**epoll 属于 I/O 多路复用，不是异步！** 数据从内核拷到用户态的过程是同步的（在 `recv` 里完成）。

### 常见并发模型对比

```text
1. 多进程（早期 Apache）
   [fork per connection]
   强：隔离性好  →  弱：开销大，C10K 问题

2. 多线程（早期 Tomcat）
   [thread per connection]
   强：比多进程轻  →  弱：大量线程切换开销

3. Reactor + 线程池（Nginx、Redis、muduo）
   [event loop] + [worker threads]
   强：一个 loop 管几万连接  →  做法：你刚学的

4. Proactor（Windows IOCP、io_uring）
   [内核异步完成 IO → 回调通知]
   强：性能极致  →  弱：跨平台困难
```

### C10K 问题

"一台服务器如何同时处理 10,000 个客户端连接？"

答案：**epoll + 非阻塞 IO + Reactor + 线程池**，这正是你学完这套课程能写的东西。

### 面试自查清单

- [ ] 三次握手、四次挥手过程能画出来
- [ ] TCP 为什么粘包？怎么解决？
- [ ] select/poll/epoll 区别和优缺点
- [ ] LT/ET 区别，ET 下为什么要非阻塞
- [ ] Reactor 和 Proactor 区别
- [ ] 线程池怎么设计（生产者-消费者）
- [ ] 能说出 I/O 多路复用 和 异步 I/O 的本质区别

---

## 知识点对照总表

| 课 | 主题            | 核心API / 关键词                                            |
| :- | :-------------- | :---------------------------------------------------------- |
| 1  | TCP/IP 基础     | 分层、三次握手、四次挥手                                    |
| 2  | socket / 阻塞IO | `socket` `bind` `listen` `accept` `recv` `send` |
| 3  | echo server     | 服务端/客户端生命周期                                       |
| 4  | select          | `FD_ZERO` `FD_SET` `select` `FD_ISSET`              |
| 5  | 自定义协议      | 长度前缀、`recv_all` `recv_msg` `send_msg`            |
| 6  | epoll           | `epoll_create1` `epoll_ctl` `epoll_wait`、LT          |
| 7  | 非阻塞IO + ET   | `O_NONBLOCK` `EPOLLET` `EAGAIN`、循环读               |
| 8  | Reactor         | Channel、EventLoop、回调、`data.ptr`                      |
| 9  | 线程池          | `std::mutex` `condition_variable`、生产者-消费者        |
| 10 | 定时器          | `timerfd`、心跳、踢线                                     |
| 11 | HTTP/1.1        | 请求行、`Content-Length`、`\r\n\r\n`                    |
| 12 | 总结            | 5种IO模型、C10K、并发模型对比、面试清单                     |
