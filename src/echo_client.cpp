#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

const int PORT = 8888;
const int BUFFER_SIZE = 1024;

int main(){
    //WSAStartup初始化
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2),&wsaData)) {return -1;}
    //创建socket
    SOCKET sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(sock==INVALID_SOCKET){
        WSACleanup();
        std::cerr<<WSAGetLastError()<<std::endl;
        return -1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(PORT);
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");//连本地

    if(connect(sock,(sockaddr*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR){//连接服务器
        //报错处理
        WSACleanup();
        closesocket(sock);
        std::cerr<<WSAGetLastError()<<std::endl;

        return -1;
    }

    //接下来：通信循环
    std::cout<<"Connected to server..."<<std::endl;
    std::string username;//获取用户名
    std::cout<<"Enter your name: ";
    std::getline(std::cin,username);
    char buf[BUFFER_SIZE];
    std::string input;
    
    while(1){
        //获取用户输入
        getline(std::cin,input);
        if(input=="quit"){
            break;
        }
        std::string msg=username+": "+input;//完整信息
        send(sock,msg.c_str(),msg.size(),0);//发送给服务器
        //这里有点不懂，可看看处理方式。直接用cout打印char，如果没有\0是不会结束字符串的，所以不能直接打印
        int bytesReceived=recv(sock,buf,BUFFER_SIZE,0);//收回声，"回声"就是服务端把收来的数据原样发回
        std::string echo(buf,bytesReceived);//用字节数构造
        std::cout<<echo<<std::endl;
    }
    closesocket(sock);
    WSACleanup();
    return 0;
}