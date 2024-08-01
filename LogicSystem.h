#pragma once

#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"

class HttpConnection;

//����һ����Ϊ HttpHandler ������
//��ʾһ������ std::shared_ptr<HttpConnection> �����Ŀɵ��ö���
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem :public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem(){}

    //���� GET ����ķ���������Ϊ�����·����һ��ָ�� HttpConnection ����Ĺ���ָ�룬���ش����Ƿ�ɹ�
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);

    //ע�� GET ���������ķ���������Ϊ����·���ʹ������
    void RegGet(std::string, HttpHandler handler);

    //���� POST ����ķ���������Ϊ�����·����һ��ָ�� HttpConnection ����Ĺ���ָ�룬���ش����Ƿ�ɹ�
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>);

    //ע�� POST ���������ķ���������Ϊ����·���ʹ������
    void RegPost(std::string url, HttpHandler handler);

private:
    //���캯����˽�л���ʵ�ֵ���ģʽ
    LogicSystem();

    //���ڴ洢 POST ����������ӳ�䣬��Ϊ����·����ֵΪ�������
    std::map<std::string, HttpHandler> _post_handlers;

    //���ڴ洢 GET ����������ӳ�䣬��Ϊ����·����ֵΪ�������
    std::map<std::string, HttpHandler> _get_handlers;
};