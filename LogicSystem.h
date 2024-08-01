#pragma once

#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"

class HttpConnection;

//定义一个名为 HttpHandler 的类型
//表示一个接收 std::shared_ptr<HttpConnection> 参数的可调用对象
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem :public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem(){}

    //处理 GET 请求的方法。参数为请求的路径和一个指向 HttpConnection 对象的共享指针，返回处理是否成功
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);

    //注册 GET 请求处理程序的方法。参数为请求路径和处理程序
    void RegGet(std::string, HttpHandler handler);

    //处理 POST 请求的方法。参数为请求的路径和一个指向 HttpConnection 对象的共享指针，返回处理是否成功
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>);

    //注册 POST 请求处理程序的方法。参数为请求路径和处理程序
    void RegPost(std::string url, HttpHandler handler);

private:
    //构造函数，私有化以实现单例模式
    LogicSystem();

    //用于存储 POST 请求处理程序的映射，键为请求路径，值为处理程序
    std::map<std::string, HttpHandler> _post_handlers;

    //用于存储 GET 请求处理程序的映射，键为请求路径，值为处理程序。
    std::map<std::string, HttpHandler> _get_handlers;
};