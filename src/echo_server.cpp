#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<functional>
#include<sys/timerfd.h>
#include<ctime>

const int PORT = 8888;
const int BUFFER_SIZE = 1024;

class EventLoop;

class Channel{//封装一个文件描述符和它的事件处理函数
public:
    Channel(EventLoop*loop,int fd);
    
    void setReadCallback(std::function<void()> cb) {readCb_=std::move(cb);}
    void setCloseCallback(std::function<void()> cb){closeCb_=std::move(cb);}
    void enableReading(bool et=true);//把自己注册到epoll
    void disableAll();//从epoll移除自己
    void handleEvent(uint32_t events);//事件发生时调用，调用相应的回调函数

    int fd() const{return fd_;}//获取文件描述符

private:
    EventLoop*loop_;
    int fd_;
    uint32_t events_=0;
    std::function<void()> readCb_;
    std::function<void()>closeCb_;
    bool added_=false;//是否已经注册到epoll了
public:
    friend class EventLoop;
    uint32_t events() const{return events_;}
    bool added()const {return added_;}
    void setAdded(bool v){added_=v;}
};

class EventLoop{
public:
    EventLoop(){
        epfd_=epoll_create1(0);
        if(epfd_<0){
            std::cerr<<"epoll_create1() failed: "<<strerror(errno)<<std::endl;
            exit(-1);//创建失败，退出程序
        }
    }
    ~EventLoop(){
        close(epfd_);
    }

    void updateChannel(Channel* channel){
        struct epoll_event ev;
        ev.events=channel->events();
        ev.data.ptr=channel;//把Channel指针存到事件数据中，方便事件发生时找到对应的Channel对象
        if(!channel->added()){//如果还没有注册到epoll，就添加
            epoll_ctl(epfd_,EPOLL_CTL_ADD,channel->fd(),&ev);
        }else{
            epoll_ctl(epfd_,EPOLL_CTL_MOD,channel->fd(),&ev);//已经注册了，就修改事件
        }
        channel->setAdded(true);

    }

    void removeChannel(Channel* channel){
        epoll_ctl(epfd_,EPOLL_CTL_DEL,channel->fd(),nullptr);
        channel->setAdded(false);//从epoll移除后，标记为未添加状态
    }
    
    void loop(){
        struct epoll_event events[64];
        while(running_){
            int n=epoll_wait(epfd_,events,64,-1);
            if(n<0) break;
            for(int i=0;i<n;i++){
                Channel*channel=static_cast<Channel*>(events[i].data.ptr);
                channel->handleEvent(events[i].events);//调用Channel的事件处理函数，根据事件类型调用相应的回调函数
            }
        }
    }
    void quit(){running_=false;}
private:
    int epfd_;
    bool running_=true;
};
//Channel类的实现
Channel::Channel(EventLoop*loop,int fd):loop_(loop),fd_(fd){}

void Channel::enableReading(bool et){
    events_=EPOLLIN|(et?EPOLLET:0);//监听可读事件，边缘触发
    loop_->updateChannel(this);
}

void Channel::disableAll(){
    loop_->removeChannel(this);
}

void Channel::handleEvent(uint32_t revents){
    if(revents&(EPOLLIN|EPOLLPRI)){//如果返回的事件包含可读事件或者紧急数据事件，就调用读回调函数
        if(readCb_) readCb_();
    }
    if(revents&(EPOLLHUP|EPOLLERR)){//如果返回的事件包含挂起事件或者错误事件，就调用关闭回调函数
        if(closeCb_) closeCb_();
    }
}



//读取数据到缓冲区，返回是否成功读取到完整消息
//返回1成功，0对端关闭，-1出错
int read_to_buf(int fd,std::string& buf){
    char temp[1024];
    while(true){
        int n=recv(fd,temp,sizeof(temp),0);
        if(n>0){
            buf.append(temp,n);
        }else if(n==0){
            return 0;//对端关闭
        }else if(errno==EAGAIN||errno==EWOULDBLOCK){
            return 1;//读完了成功
        }else{
            return -1;//出错
        }
    }
}

//从缓冲区提取完整消息，返回是否成功提取
//返回消息长度，0表示数据不够等下一次，-1表示长度非法
int parse_msg(std::string& buf,char*out,int outSize){
    if(buf.size()<4) return 0;//头部没有读全

    int netLen;
    memcpy(&netLen,buf.data(),4);
    int msgLen=ntohl(netLen);//转换为主机字节序

    if(msgLen>outSize) return -1;//消息太大
    if((int)buf.size()<4+msgLen) return 0;//消息体没有读全
    
    memcpy(out,buf.data()+4,msgLen);//复制消息体到输出缓冲区
    buf.erase(0,4+msgLen);//从缓冲区删除取走的消息
    return msgLen;

}

void send_msg(int s, const char* data, int len) {
    int netLen = htonl(len);
    send(s, (char*)&netLen, 4, 0);
    send(s, data, len, 0);
}

int main() {
    //设置标准输入stdin为非阻塞
    fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL,0)|O_NONBLOCK);

    //listenSockaa创建时提供非阻塞
    int listenSock = socket(AF_INET, SOCK_STREAM| SOCK_NONBLOCK, IPPROTO_TCP);

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

    struct ClientInfo {
        int sock;
        std::string name;
        std::string inBuf;//加入缓冲区，用于存储未完整接收的消息数据
        Channel*channel;//每个客户端对应一个Channel对象，用于管理这个客户端套接字的事件
        time_t lastActiveTime=time(nullptr);//记录客户端的最后活跃时间，用于实现超时断开功能
    };
    std::vector<ClientInfo> clients;
    //创建EventLoop对象，管理事件循环和Channel对象
    EventLoop loop;
    //--stdin Channel--
    Channel stdinChannel(&loop,STDIN_FILENO);
    //--timer Channel--
    int tfd=timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
    struct itimerspec ts{};//设置定时器的初始到期时间和间隔时间
    ts.it_value.tv_sec=10;//初始到期时间为10秒
    ts.it_interval.tv_sec=10;//间隔时间为10秒
    timerfd_settime(tfd,0,&ts,nullptr);//启动定时器

    Channel timerChannel(&loop,tfd);
    timerChannel.enableReading(false);//监听定时器的可读事件，水平触发LT就行了
    timerChannel.setReadCallback([&](){
        uint64_t exp;//64位
        read(tfd,&exp,sizeof(exp));//消费事件

        time_t now=time(nullptr);
        std::cout<<"[INFO] 在线： "<<clients.size()<<" 人"<<std::endl;

        //踢掉30秒没法消息的客户端
        for(int i=(int)clients.size()-1;i>=0;i--){
            if(now-clients[i].lastActiveTime>30){
                if(!clients[i].name.empty()){
                    std::string leaveMsg=clients[i].name+" timed out.";
                    std::cout<<leaveMsg<<std::endl;
                    for(const ClientInfo&client:clients){
                        if(client.sock!=clients[i].sock){
                            send_msg(client.sock,leaveMsg.c_str(),leaveMsg.size());//
                        }
                    }
                }
                clients[i].channel->disableAll();
                close(clients[i].sock);
                delete clients[i].channel;
                clients.erase(clients.begin()+i);
            }
        }
    });


    stdinChannel.enableReading(false);//监听标准输入的可读事件，水平触发就行了
    stdinChannel.setReadCallback([&](){
        std::string cmd;
        std::getline(std::cin,cmd);
        if(cmd=="quit") loop.quit();
    });
    //--listen Channel--
    Channel listenChannel(&loop,listenSock);
    listenChannel.enableReading(true);//监听监听套接字的可读事件用ET,true表示边缘触发，效率更高
    listenChannel.setReadCallback([&](){
        //accept,直到EAGAIN
        while(true){
            sockaddr_in clientAddr{};
            socklen_t addrLen=sizeof(clientAddr);
            int clientSock=accept4(listenSock,(sockaddr*)&clientAddr,&addrLen,SOCK_NONBLOCK);
            //如果成功接受到一个新连接，就创建一个新的Channel对象来管理这个客户端套接字，并设置相应的读回调函数来处理客户端消息。同时，将客户端信息添加到clients向量中，以便后续处理。
            if(clientSock>=0){
                std::cout<<"Client connected: "<<inet_ntoa(clientAddr.sin_addr)<<std::endl;

                Channel*clientChannel=new Channel(&loop,clientSock);
                clientChannel->enableReading();
                clients.push_back({clientSock,"","",clientChannel});
                //设置客户端Channel的读回调函数，处理客户端消息
                //其余部分捕获列表都用引用捕获，clientChannel用值捕获，确保在回调函数中可以正确访问这个Channel对象
                //每次循环一次后指针变量本身会被销毁，但它的值（地址）被 lambda 保存下来了
                clientChannel->setReadCallback([&,clientChannel](){
                    //找到idx
                    int idx=-1;
                    for(int k=0;k<(int)clients.size();k++){
                        if(clients[k].sock==clientChannel->fd()){idx=k;break;}//根据Channel的文件描述符找到对应的客户端信息
                    }
                    if(idx<0) return;//没找到
                    //step1:把socket的数据全部读到inBuf
                    clients[idx].lastActiveTime=time(nullptr);//收到有效信息就刷新
                    int ret=read_to_buf(clients[idx].sock,clients[idx].inBuf);
                    if(ret<=0){
                        if(!clients[idx].name.empty()){//如果客户端已经设置了名字
                            //广播离开消息
                            std::string leaveMsg=clients[idx].name+" has left the chat.";
                            std::cout<<leaveMsg<<std::endl;
                            for(const ClientInfo& client:clients){
                                if(client.sock!=clients[idx].sock){
                                    send_msg(client.sock,leaveMsg.c_str(),leaveMsg.size());
                                }
                            }
                        }

                        clientChannel->disableAll();
                        close(clients[idx].sock);
                        delete clientChannel;//客户端断开连接后，删除对应的Channel对象，释放资源
                        clients.erase(clients.begin()+idx);
                        return;
                    }
                    //parse消息
                    char buf[BUFFER_SIZE];
                    while(true){
                        int msgLen=parse_msg(clients[idx].inBuf,buf,BUFFER_SIZE);
                        if(msgLen<=0){break;}//没有完整消息了
                        std::string msg(buf,msgLen);
                        //如果客户端还没有设置名字，并且消息以"/name:"开头，就把后面的部分作为名字设置上去，并广播加入消息
                        if(clients[idx].name.empty()&&msg.rfind("/name:",0)==0){
                            clients[idx].name=msg.substr(6);
                            std::cout<<clients[idx].name<<" has joined the chat."<<std::endl;
                        }
                        else{
                            for(const ClientInfo& client:clients){
                                if(client.sock!=clients[idx].sock){
                                    send_msg(client.sock,msg.c_str(),msg.size());
                                }
                            }
                            std::cout<<"Broadcast: "<<msg<<std::endl;
                        }
                    }

                });
            }else if(errno==EAGAIN){
                break;//没有更多连接了
            }else{
                std::cerr<<"accept4() failed: "<<strerror(errno)<<std::endl;
                break;
            }
        }

    });
    //--启动--
    std::cout<<"Listening on port "<<PORT<<"..."<<std::endl;
    loop.loop();
    //--清理--
    for(ClientInfo&client:clients){
        close(client.sock);
        delete client.channel;//删除Channel对象，释放资源
    }
    close(listenSock);
    close(tfd);//清理计时器
    return 0;
}
