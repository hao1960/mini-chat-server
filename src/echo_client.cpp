#include "protocol.h"
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

std::atomic<bool> running{true};

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
