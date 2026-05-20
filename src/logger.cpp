#include"logger.h"
#include<iostream>
#include<ctime>


//-- logger --
Logger& Logger::instance(){
    static Logger logger;
    return logger;
}

void Logger::init(const std::string& filePath, Level minLevel){
    logFile.open(filePath,std::ios::app);
    if(!logFile){
        std::cerr<<"Failed to open log file: "<<filePath<<std::endl;
        exit(-1);
    }
    this->minLevel=minLevel;


}

//核心方法：格式化+过滤+双写
void Logger::log(Level lv,const std::string& msg){
    if(lv<minLevel) return;//过滤掉低于最低等级的日志
    //格式化日志：时间+等级+消息
    std::string levelStr;
    switch(lv){
        case DEBUG: levelStr="DEBUG";break;
        case INFO: levelStr="INFO";break;
        case WARN: levelStr="WARN";break;
        case ERROR: levelStr="ERROR";break;
    }

    std::time_t now=std::time(nullptr);//获取当前时间
    char timeBuf[20];//格式化时间为字符串
    std::strftime(timeBuf,sizeof(timeBuf),"%Y-%m-%d %H:%M:%S",std::localtime(&now));

    std::string logLine=std::string(timeBuf)+" ["+levelStr+"] "+msg+'\n';
    std::cout<<logLine;//控制台输出,不用每次都flush
    if(logFile.is_open()){
        logFile<<logLine<<std::flush;//文件输出,每次都flush确保写入
    }
}

//四个便捷方法（里面调用log）
void Logger::debug(const std::string& msg){
    log(DEBUG,msg);
}
void Logger::info(const std::string& msg){
    log(INFO,msg);
}
void Logger::warn(const std::string& msg){
    log(WARN,msg);
}
void Logger::error(const std::string& msg){
    log(ERROR,msg);
}