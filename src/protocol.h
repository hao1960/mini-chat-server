#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

class Channel;
const int PORT = 8888;
const int BUFFER_SIZE = 1024;

struct ClientInfo {
    int sock;
    std::string name;
    std::string inBuf;
    Channel* channel = nullptr;
    time_t lastActiveTime = time(nullptr);
};

// 服务端用的协议函数
int  read_to_buf(int fd, std::string& buf);
int  parse_msg(std::string& buf, char* out, int outSize);
void send_msg(int s, const char* data, int len);
bool handle_http_request(int fd, const std::string& inBuf, std::vector<ClientInfo>& clients);

// 客户端用的协议函数（阻塞式）
int  recv_all(int s, char* buf, int len);
int  recv_msg(int s, char* buf, int bufSize);
