#include "protocol.h"
#include <iostream>
#include <sys/socket.h>

// ── 服务端 ──

int read_to_buf(int fd, std::string& buf) {
    char temp[1024];
    while (true) {
        int n = recv(fd, temp, sizeof(temp), 0);
        if (n > 0) {
            buf.append(temp, n);
        } else if (n == 0) {
            return 0;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 1;
        } else {
            return -1;
        }
    }
}

int parse_msg(std::string& buf, char* out, int outSize) {
    if (buf.size() < 4) return 0;

    uint16_t magic;
    memcpy(&magic, buf.data(), 2);
    if (ntohs(magic) != 0xABCD) return -2;

    uint16_t netLen;
    memcpy(&netLen, buf.data() + 2, 2);
    int msgLen = ntohs(netLen);

    if (msgLen > outSize || msgLen <= 0) return -1;
    if ((int)buf.size() < 4 + msgLen) return 0;

    memcpy(out, buf.data() + 4, msgLen);
    buf.erase(0, 4 + msgLen);
    return msgLen;
}

void send_msg(int s, const char* data, int len) {
    uint16_t magic = htons(0xABCD);
    send(s, (char*)&magic, 2, 0);
    uint16_t netLen = htons((uint16_t)len);
    send(s, (char*)&netLen, 2, 0);
    send(s, data, len, 0);
}

bool handle_http_request(int fd, const std::string& inBuf, std::vector<ClientInfo>& clients) {
    if (inBuf.rfind("GET ", 0) != 0) return false;

    size_t start = inBuf.find(' ') + 1;
    size_t end   = inBuf.find(' ', start);
    std::string path = inBuf.substr(start, end - start);

    std::string body, status;
    if (path == "/") {
        status = "200 OK";
        int online = 0;
        for (const auto& c : clients)
            if (!c.name.empty()) online++;
        body  = "<html><head><meta charset='utf-8'><title>Chat Server</title></head><body>";
        body += "<h1>Mini Chat Server</h1><h3>在线用户 (" + std::to_string(online) + ")</h3><ul>";
        for (const auto& c : clients)
            if (!c.name.empty())
                body += "<li>" + c.name + "</li>";
        body += "</ul></body></html>";
    } else {
        status = "404 Not Found";
        body   = "404 Not Found";
    }

    std::string resp = "HTTP/1.1 " + status + "\r\nContent-Type: text/html; charset=utf-8\r\n"
                       "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    send(fd, resp.c_str(), resp.size(), 0);
    return true;
}

// ── 客户端（阻塞式） ──

int recv_all(int s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
}

int recv_msg(int s, char* buf, int bufSize) {
    uint16_t magic;
    if (recv_all(s, (char*)&magic, 2) <= 0) return -1;
    if (ntohs(magic) != 0xABCD) return -2;

    uint16_t netLen;
    if (recv_all(s, (char*)&netLen, 2) <= 0) return -1;
    int len = ntohs(netLen);

    if (len > bufSize || len <= 0) return -1;
    if (recv_all(s, buf, len) < 0) return -1;
    return len;
}
