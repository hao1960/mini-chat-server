#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>


const int PORT = 8888;
const int BUFFER_SIZE = 1024;

std::atomic<bool> running{true};

int recv_all(int s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
}
//
int recv_msg(int s, char* buf, int bufSize) {
    uint16_t magic;
    if(recv_all(s,(char*)&magic,2)<=0) return -1;
    if(ntohs(magic)!=0xABCD) return -2;//非法协议

    uint16_t netLen;
    if(recv_all(s,(char*)&netLen,2)<=0) return -1;
    int len=ntohs(netLen);

    if(len>bufSize||len<=0) return -1;
    if(recv_all(s,buf,len)<0) return -1;
    return len;
}

void send_msg(int s, const char* data, int len) {
    uint16_t  magic=htons(0xABCD);
    send(s,(char*)&magic,2,0);
    uint16_t netLen=htons((uint16_t)len);
    send(s,(char*)&netLen,2,0);
    send(s,data,len,0);
}

void recv_loop(int sock, std::atomic<bool>& running) {
    char buf[BUFFER_SIZE];
    while (running) {
        int n = recv_msg(sock, buf, BUFFER_SIZE);
        if (n <= 0) break;//-1,-2,0都断开
        std::cout << std::string(buf, n) << std::endl;
    }
    running = false;
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "connect() failed: " << strerror(errno) << std::endl;
        close(sock);
        return -1;
    }

    std::cout << "Connected to server..." << std::endl;
    std::string username;
    std::cout << "Enter your name: ";
    std::getline(std::cin, username);

    std::string loginMsg = "/name:" + username;
    send_msg(sock, loginMsg.c_str(), loginMsg.size());

    std::string input;
    std::thread t(recv_loop, sock, std::ref(running));

    while (true) {
        std::getline(std::cin, input);
        if (input == "quit") break;
        std::string msg = username + ": " + input;
        send_msg(sock, msg.c_str(), msg.size());
    }

    running = false;
    shutdown(sock, SHUT_RDWR);
    t.join();
    close(sock);
    return 0;
}
