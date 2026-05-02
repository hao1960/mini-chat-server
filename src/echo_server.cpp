#include <iostream>
#include <string>
#include<vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

const int PORT = 8888;
const int BUFFER_SIZE = 1024;

int main(){
    //Winsock初始化+创建socket
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)){return -1;}
    //监听socket
    SOCKET listenSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if(listenSock==INVALID_SOCKET){
        WSACleanup();
        std::cerr<<WSAGetLastError()<<std::endl;
        return -1;
    }
    
    //接下来：bind()+listen()
    sockaddr_in serverAddr{};
    serverAddr.sin_family=AF_INET;
    //监听所有网卡
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    //转网络字节序
    serverAddr.sin_port=htons(PORT);
    
    if(bind(listenSock,(sockaddr*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR){
        std::cerr<<WSAGetLastError()<<std::endl;
        //关socket
        closesocket(listenSock);
        //cleanup
        WSACleanup();
        return -1;
    }

    if(listen(listenSock,SOMAXCONN)==SOCKET_ERROR){
        std::cerr<<WSAGetLastError()<<std::endl;
        //关socket
        closesocket(listenSock);
        //cleanup
        WSACleanup();
        return -1;
    }else{
        std::cout<<"Listening on port 8888..."<<std::endl;
    }


    //select方法处理多客户端
    std::vector<SOCKET> clients;//保存所有已连接的客户端

    while(true){
        fd_set readSet;
        FD_ZERO(&readSet);

        //1.把监听socket加入集合
        FD_SET(listenSock,&readSet);
        int maxFd=(int)listenSock;

        //2.把所有客户端socket也加入集合
        for(SOCKET s:clients){
            FD_SET(s,&readSet);
            if(s>maxFd) maxFd=(int)s;
        }

        //3.select()阻塞等待
        int ret=select(maxFd+1,&readSet,NULL,NULL,NULL);
        if(ret==SOCKET_ERROR){
            std::cerr<<"select() failed: "<<WSAGetLastError()<<std::endl;
            break;
        }
        //4.检查监听socket--有新连接?
        if(FD_ISSET(listenSock,&readSet)){
            //accept+加入clients列表
            sockaddr_in clientAddr{};
            int addrLen=sizeof(clientAddr);
            SOCKET clientSock=accept(listenSock,(sockaddr*)&clientAddr,&addrLen);
            if(clientSock!=INVALID_SOCKET){
                //二进制转字符串函数，常用于服务端打印客户端ip
                char*ip=inet_ntoa(clientAddr.sin_addr);
                std::cout<<"Client connected: "<<ip<<std::endl;
                clients.push_back(clientSock);
            }
            else{
                //accept失败不要退出循环，打印错误即可
                std::cerr<<"accept() failed: "<<WSAGetLastError()<<std::endl;
            }
        }
        //5.检查每个客户端-有消息？
        for(int i=0;i<(int)clients.size();i++){
            if(FD_ISSET(clients[i],&readSet)){
                //recv+处理
                char buf[BUFFER_SIZE];
                int bytesReceived =recv(clients[i],buf,BUFFER_SIZE,0);
                //三种情况处理
                if(bytesReceived>0){//收到消息，广播给其他客户端
                    for(SOCKET s:clients){
                        if(s!=clients[i]){//不发给自己
                            send(s,buf,bytesReceived,0);
                        }
                    }
                    //打印到服务端控制台
                    std::string msg(buf,bytesReceived);
                    std::cout<<"Broadcast: "<<msg<<std::endl;
                }
                else {//客户端断开，关socket并从列表移除
                    closesocket(clients[i]);
                    clients.erase(clients.begin()+i);
                    i--;//删除了当前元素，下次循环继续检查同一个i位置
                }
            }
        }
    }
    //关闭
    for(SOCKET s:clients) closesocket(s);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}