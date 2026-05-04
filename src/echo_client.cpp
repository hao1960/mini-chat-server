#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include<thread>
#include<atomic>
#pragma comment(lib,"ws2_32.lib")

const int PORT = 8888;
const int BUFFER_SIZE = 1024;

std::atomic<bool> running{true};

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

//接收线程函数
void recv_loop(SOCKET sock,std::atomic<bool>& running){
    char buf[BUFFER_SIZE];
    while(running){
        int n=recv_msg(sock,buf,BUFFER_SIZE);
        if(n<=0) break;//断开错误
        std::cout<<std::string(buf,n)<<std::endl;
    }
    running = false;
}

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
    //连接成功后，发送用户名注册
    std::string loginMsg="/name:"+username;
    send_msg(sock,loginMsg.c_str(),loginMsg.size());

    std::string input;
    
    //while之前启动线程
    std::thread t(recv_loop,sock,std::ref(running));

    while(1){
        //获取用户输入
        getline(std::cin,input);
        if(input=="quit"){
            break;
        }
        std::string msg=username+": "+input;//完整信息
        send_msg(sock,msg.c_str(),msg.size());//发送给服务器
        // //这里有点不懂，可看看处理方式。直接用cout打印char，如果没有\0是不会结束字符串的，所以不能直接打印
        // int bytesReceived=recv_msg(sock,buf,BUFFER_SIZE);//收回声，"回声"就是服务端把收来的数据原样发回
        // std::string echo(buf,bytesReceived);//用字节数构造
        // std::cout<<echo<<std::endl;
    }
    //线程结束后做清理
    t.join();
    
    closesocket(sock);
    WSACleanup();
    return 0;
}