#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

const int PORT = 8888;
const int BUFFER_SIZE = 1024;

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
    int netLen;
    if (recv_all(s, (char*)&netLen, 4) <= 0) return -1;
    int len = ntohl(netLen);
    if (len > bufSize) return -1;
    if (recv_all(s, buf, len) < 0) return -1;
    return len;
}

void send_msg(int s, const char* data, int len) {
    int netLen = htonl(len);
    send(s, (char*)&netLen, 4, 0);
    send(s, data, len, 0);
}

int main() {
    int listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock < 0) {
        std::cerr << "socket() failed: " << strerror(errno) << std::endl;
        return -1;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "bind() failed: " << strerror(errno) << std::endl;
        close(listenSock);
        return -1;
    }

    if (listen(listenSock, SOMAXCONN) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << std::endl;
        close(listenSock);
        return -1;
    }
    std::cout << "Listening on port 8888..." << std::endl;

    struct ClientInfo {
        int sock;
        std::string name;
    };
    std::vector<ClientInfo> clients;

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);

        FD_SET(listenSock, &readSet);
        int maxFd = listenSock;

        for (const ClientInfo& client : clients) {
            FD_SET(client.sock, &readSet);
            if (client.sock > maxFd) maxFd = client.sock;
        }

        int ret = select(maxFd + 1, &readSet, NULL, NULL, NULL);
        if (ret < 0) {
            std::cerr << "select() failed: " << strerror(errno) << std::endl;
            break;
        }

        if (FD_ISSET(listenSock, &readSet)) {
            sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);
            int clientSock = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
            if (clientSock >= 0) {
                char* ip = inet_ntoa(clientAddr.sin_addr);
                std::cout << "Client connected: " << ip << std::endl;
                clients.push_back({clientSock, ""});
            } else {
                std::cerr << "accept() failed: " << strerror(errno) << std::endl;
            }
        }

        for (int i = 0; i < (int)clients.size(); i++) {
            if (FD_ISSET(clients[i].sock, &readSet)) {
                char buf[BUFFER_SIZE];
                int bytesReceived = recv_msg(clients[i].sock, buf, BUFFER_SIZE);
                if (bytesReceived > 0) {
                    std::string msg(buf, bytesReceived);
                    if (clients[i].name.empty() && msg.rfind("/name:", 0) == 0) {
                        clients[i].name = msg.substr(6);
                        std::cout << clients[i].name << " joined" << std::endl;
                        continue;
                    }

                    for (const ClientInfo& client : clients) {
                        if (client.sock != clients[i].sock) {
                            send_msg(client.sock, buf, bytesReceived);
                        }
                    }
                    std::cout << "Broadcast: " << msg << std::endl;
                } else {
                    if (!clients[i].name.empty()) {
                        std::string leaveMsg = clients[i].name + " left";
                        std::cout << leaveMsg << std::endl;
                        for (const ClientInfo& client : clients) {
                            if (client.sock != clients[i].sock) {
                                send_msg(client.sock, leaveMsg.c_str(), leaveMsg.size());
                            }
                        }
                    }

                    close(clients[i].sock);
                    clients.erase(clients.begin() + i);
                    i--;
                }
            }
        }
    }

    for (const ClientInfo& client : clients) close(client.sock);
    close(listenSock);
    return 0;
}
