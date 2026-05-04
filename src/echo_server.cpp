#include <iostream>
#include <string>
#include<vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

const int PORT = 8888;
const int BUFFER_SIZE = 1024;
//接收len字节，处理半包情况
int recv_all(SOCKET s,char*buf,int len){
    int total=0;
    while(total<len){
        int n=recv(s,buf+total,len-total,0);
        if(n<=0) return -1;//连接断开或出错
        total+=n;
    }
    return total;
}
//按协议收一条完整消息
int recv_msg(SOCKET s,char* buf,int bufSize){
    int netLen;
    if(recv_all(s,(char*)&netLen,4)<=0) return -1;//先收4字节长度
    int len=ntohl(netLen);//转主机字节序
    if(len>bufSize) return -1;//消息太大，缓冲区不够
    if(recv_all(s,buf,len)<0) return -1;//再接收消息内容
    return len;
}
//按协议发一条消息
void send_msg(SOCKET s,const char*data,int len){
    int netLen=htonl(len);//转网络字节序
    send(s,(char*)&netLen,4,0);//先发4字节长度
    send(s,data,len,0);//再发消息内容
}
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
    struct ClientInfo{
        SOCKET sock;
        std::string name;
    };
    std::vector<ClientInfo> clients;//保存所有已连接的客户端

    while(true){
        fd_set readSet;
        FD_ZERO(&readSet);

        //1.把监听socket加入集合
        FD_SET(listenSock,&readSet);
        int maxFd=(int)listenSock;

        //2.把所有客户端socket也加入集合
        for(const ClientInfo& client:clients){
            FD_SET(client.sock,&readSet);
            if(client.sock>maxFd) maxFd=(int)client.sock;
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
                clients.push_back({clientSock,""});//先把ip当名字，后面可以让客户端发个消息改名字
            }
            else{
                //accept失败不要退出循环，打印错误即可
                std::cerr<<"accept() failed: "<<WSAGetLastError()<<std::endl;
            }
        }
        //5.检查每个客户端-有消息？
        for(int i=0;i<(int)clients.size();i++){
            if(FD_ISSET(clients[i].sock,&readSet)){
                //recv+处理
                char buf[BUFFER_SIZE];
                int bytesReceived =recv_msg(clients[i].sock,buf,BUFFER_SIZE);
                //三种情况处理
                if(bytesReceived>0){//收到消息，广播给其他客户端。需要判断是不是登录消息
                    std::string msg(buf,bytesReceived);
                    //如果名字为空且消息以/name:开头，说明这是登录消息，提取名字，不广播
                    if(clients[i].name.empty()&&msg.rfind("/name:",0)==0){
                        clients[i].name=msg.substr(6);//截取名字部分
                        std::cout<<clients[i].name<<" joined"<<std::endl;
                        continue;//跳过广播
                    }

                    //否则正常广播
                    for(const ClientInfo& client:clients){
                        if(client.sock!=clients[i].sock){//不发给自己
                            send_msg(client.sock,buf,bytesReceived);
                        }
                    }
                    //打印到服务端控制台
                    std::cout<<"Broadcast: "<<msg<<std::endl;
                }
                else {//客户端断开，关socket并从列表移除。断开通知其他客户端
                    //广播断开通知
                    if(!clients[i].name.empty()){
                        std::string leaveMsg=clients[i].name+" left";
                        std::cout<<leaveMsg<<std::endl;
                        for(const ClientInfo& client:clients){
                            if(client.sock!=clients[i].sock){
                                send_msg(client.sock,leaveMsg.c_str(),leaveMsg.size());
                            }
                        }
                    }



                    closesocket(clients[i].sock);
                    clients.erase(clients.begin()+i);
                    i--;//删除了当前元素，下次循环继续检查同一个i位置
                }
            }
        }
    }
    //关闭
    for(const ClientInfo& client:clients) closesocket(client.sock);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}