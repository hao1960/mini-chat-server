#pragma once

#include<iostream>
#include<fstream>
#include<string>


class Logger{
public:
    enum Level{DEBUG=0,INFO,WARN,ERROR};

    //单例：返回全局唯一实例
    static Logger& instance();

    //初始化：打开日志文件+设置最低等级
    void init(const std::string& filePath,Level minLevel=INFO);

    //核心方法：格式化+过滤+双写
    void log(Level lv, const std::string& msg);

    //四个便捷方法（里面调用log）
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    //构造函数私有，防止外部new
    Logger()=default;

    std::ofstream logFile;//文件日志流
    Level minLevel=INFO;//最低日志等级
};