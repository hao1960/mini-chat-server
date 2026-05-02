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

| 特性 | 说明 |
| :--- | :--- |
| 面向连接 | 通信前先建立连接（三次握手） |
| 可靠 | 丢包重传，乱序重排 |
| 字节流 | **没有消息边界**——粘包问题的根源 |

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

| 参数 | 值 | 含义 |
| :--- | :--- | :--- |
| 地址族 | `AF_INET` | 使用 IPv4 |
| 类型 | `SOCK_STREAM` | 流式 socket（TCP） |
| 协议 | `IPPROTO_TCP` | TCP 协议 |

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

| 函数 | 阻塞原因 | 什么时候返回 |
| :--- | :--- | :--- |
| `accept()` | 无客户端连接 | 有客户端连上来 |
| `connect()` | 三次握手未完成 | 连接成功或失败 |
| `recv()` | 内核缓冲区为空 | 收到数据或对端关闭 |
| `send()` | 内核缓冲区满了 | 缓冲区有空间了 |

**为什么阻塞是 chat server 的问题：**

单线程阻塞模型下，如果客户端 A 不说话，`recv(A)` 一直阻塞，B 和 C 都连不上。解决方案：**多线程**或 **I/O 多路复用（select/poll）**。

### 字节序

TCP/IP 协议规定网络传输用**大端（网络字节序）**，x86 CPU 用**小端**，所以需要转换：

| 函数 | 含义 |
| :--- | :--- |
| `htons()` | Host TO Network Short（16 位端口） |
| `htonl()` | Host TO Network Long（32 位 IP） |
| `ntohs()` | Network TO Host Short |
| `ntohl()` | Network TO Host Long |

---

## 第3课：echo server/client 实战

### 程序员需要记住的核心函数

#### 服务端生命周期

```text
WSAStartup → socket → bind → listen → accept → recv/send → closesocket → WSACleanup
```

| 函数 | 必记 | 说明 |
| :--- | :--- | :--- |
| `WSAStartup()` | ★ | Winsock 初始化，程序最开头调用一次 |
| `socket()` | ★ | 创建 socket，返回 `SOCKET` 句柄 |
| `bind()` | ★ | 绑定端口和 IP 到 socket |
| `listen()` | ★ | 开始监听，第二个参数传 `SOMAXCONN` |
| `accept()` | ★ | 接受客户端连接，返回**新的**客户端 socket |
| `recv()` / `send()` | ★ | 收发数据 |
| `closesocket()` | ★ | 关闭 socket |
| `WSACleanup()` | ★ | 清理 Winsock，程序最后调用一次 |

#### 客户端生命周期

```text
WSAStartup → socket → connect → send/recv → closesocket → WSACleanup
```

| 函数 | 必记 | 说明 |
| :--- | :--- | :--- |
| `connect()` | ★ | 连接服务器，传入服务器地址和端口 |

#### 辅助函数（理解含义即可）

| 函数 | 说明 |
| :--- | :--- |
| `htons()` | 端口转网络字节序（记住这个就行） |
| `inet_addr()` | 字符串 IP → 二进制，如 `"127.0.0.1"` |
| `inet_ntoa()` | 二进制 IP → 字符串（只在服务端打印客户端 IP 时用） |
| `WSAGetLastError()` | 获取错误码，配合 `std::cerr` 打印 |

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

| 参数 | 作用 |
| :--- | :--- |
| `readfds` | 要监视"可读"的 socket 集合 |
| `timeout` | `NULL` = 无限等待，`{0,0}` = 立即返回 |

### fd_set 操作宏（必须记住）

| 宏 | 作用 |
| :--- | :--- |
| `FD_ZERO(&set)` | 清空集合 |
| `FD_SET(s, &set)` | 把 socket s 加入集合 |
| `FD_ISSET(s, &set)` | select 返回后检查 s 是否有事件 |
| `FD_CLR(s, &set)` | 从集合移除 s |

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
