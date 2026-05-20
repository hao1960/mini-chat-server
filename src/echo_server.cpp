#include "reactor.h"
#include "protocol.h"
#include "logger.h"
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <ctime>

int main() {
    //logger初始化
    Logger::instance().init("chat.log", Logger::DEBUG);
    // stdin 非阻塞
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);

    // 创建 listen socket
    int listenSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (listenSock < 0) {
        Logger::instance().error("socket() failed: "+std::string(strerror(errno)));
        return -1;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        Logger::instance().error("bind() failed: "+std::string(strerror(errno)));
        close(listenSock);
        return -1;
    }

    if (listen(listenSock, SOMAXCONN) < 0) {
        Logger::instance().error("listen() failed: "+std::string(strerror(errno)));
        close(listenSock);
        return -1;
    }

    std::vector<ClientInfo> clients;
    EventLoop loop;

    // ── stdin Channel ──
    Channel stdinChannel(&loop, STDIN_FILENO);
    stdinChannel.enableReading(false);
    stdinChannel.setReadCallback([&]() {
        std::string cmd;
        std::getline(std::cin, cmd);
        if (cmd == "quit") loop.quit();
    });

    // ── timer Channel ──
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    struct itimerspec ts{};
    ts.it_value.tv_sec = 10;
    ts.it_interval.tv_sec = 10;
    timerfd_settime(tfd, 0, &ts, nullptr);

    Channel timerChannel(&loop, tfd);
    timerChannel.enableReading(false);
    timerChannel.setReadCallback([&]() {
        uint64_t exp;
        read(tfd, &exp, sizeof(exp));

        time_t now = time(nullptr);
        Logger::instance().info("在线: " + std::to_string(clients.size()) + " 人");
        for (int i = (int)clients.size() - 1; i >= 0; i--) {
            if (now - clients[i].lastActiveTime > 30) {
                if (!clients[i].name.empty()) {
                    std::string leaveMsg = clients[i].name + " timed out.";
                    Logger::instance().info(leaveMsg);
                    for (const auto& c : clients)
                        if (c.sock != clients[i].sock)
                            send_msg(c.sock, leaveMsg.c_str(), leaveMsg.size());
                }
                clients[i].channel->disableAll();
                close(clients[i].sock);
                delete clients[i].channel;
                clients.erase(clients.begin() + i);
            }
        }
    });

    // ── listen Channel ──
    Channel listenChannel(&loop, listenSock);
    listenChannel.enableReading(true);
    listenChannel.setReadCallback([&]() {
        while (true) {
            sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);
            int clientSock = accept4(listenSock, (sockaddr*)&clientAddr, &addrLen, SOCK_NONBLOCK);

            if (clientSock >= 0) {
                Logger::instance().info("Client connected: "+std::string(inet_ntoa(clientAddr.sin_addr)));
                Channel* clientChannel = new Channel(&loop, clientSock);
                clientChannel->enableReading();
                clients.push_back({clientSock, "", "", clientChannel});

                clientChannel->setReadCallback([&, clientChannel]() {
                    int idx = -1;
                    for (int k = 0; k < (int)clients.size(); k++)
                        if (clients[k].sock == clientChannel->fd()) { idx = k; break; }
                    if (idx < 0) return;

                    clients[idx].lastActiveTime = time(nullptr);
                    int ret = read_to_buf(clients[idx].sock, clients[idx].inBuf);
                    if (ret <= 0) {
                        if (!clients[idx].name.empty()) {
                            std::string leaveMsg = clients[idx].name + " has left the chat.";
                            Logger::instance().info(leaveMsg);
                            for (const auto& c : clients)
                                if (c.sock != clients[idx].sock)
                                    send_msg(c.sock, leaveMsg.c_str(), leaveMsg.size());
                        }
                        clientChannel->disableAll();
                        close(clients[idx].sock);
                        delete clientChannel;
                        clients.erase(clients.begin() + idx);
                        return;
                    }

                    if (handle_http_request(clients[idx].sock, clients[idx].inBuf, clients)) {
                        clientChannel->disableAll();
                        close(clients[idx].sock);
                        delete clientChannel;
                        clients.erase(clients.begin() + idx);
                        return;
                    }

                    char buf[BUFFER_SIZE];
                    while (true) {
                        int msgLen = parse_msg(clients[idx].inBuf, buf, BUFFER_SIZE);
                        if (msgLen == -2) {
                            clientChannel->disableAll();
                            close(clients[idx].sock);
                            delete clientChannel;
                            clients.erase(clients.begin() + idx);
                            return;
                        }
                        if (msgLen <= 0) break;

                        std::string msg(buf, msgLen);
                        if (clients[idx].name.empty() && msg.rfind("/name:", 0) == 0) {
                            clients[idx].name = msg.substr(6);
                            Logger::instance().info(clients[idx].name + " joined");
                        } else {
                            for (const auto& c : clients)
                                if (c.sock != clients[idx].sock)
                                    send_msg(c.sock, msg.c_str(), msg.size());
                            Logger::instance().info("Broadcast: " + msg);
                        }
                    }
                });

            } else if (errno == EAGAIN) {
                break;
            } else {
                Logger::instance().error("accept4() failed: "+std::string(strerror(errno)));
                break;
            }
        }
    });

    // ── 启动 ──
    Logger::instance().info("Listening on port " + std::to_string(PORT) + "...");
    loop.loop();

    for (auto& c : clients) { close(c.sock); delete c.channel; }
    close(listenSock);
    close(tfd);
    return 0;
}
