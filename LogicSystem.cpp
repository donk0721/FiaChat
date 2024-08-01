#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
/*
ע�ᴦ�� GET ����Ĵ������
url�������·����
handler�����������һ�� HttpHandler ���ͣ���һ������ std::shared_ptr<HttpConnection> �����Ŀɵ��ö���
*/
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    _get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    _post_handlers.insert(make_pair(url, handler));
}


LogicSystem::LogicSystem() {
    //���� RegGet ����ע��һ��·��Ϊ /get_test �� GET ���������
    //���������һ�� lambda ���ʽ�����յ� /get_test ����ʱ������Ӧ����д�� "receive get_test req"
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
        int i = 0;
        for (auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(connection->_response.body()) << ", " << " value is " << elem.second << std::endl;
        }
        });

    //ʹ�� RegPost ����ע��һ������·��Ϊ /get_varifycode �� POST ����� Lambda ����
    //Lambda �����Ĳ�����һ�� std::shared_ptr<HttpConnection>����ʾ���ӵ�������
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        //ʹ�� boost::beast::buffers_to_string �������������ת��Ϊ�ַ��� body_str
        //������յ�������������
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;

        //������Ӧ����������Ϊ text/json
        connection->_response.set(http::field::content_type, "text/json");

        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;

        //ʹ�� reader.parse �������� body_str��������洢�� src_root ��
        //parse_success ��ʾ�����Ƿ�ɹ�
        bool parse_success = reader.parse(body_str, src_root);

        // �������ʧ�ܣ����������Ϣ��������Ӧ�еĴ����롣
        // ��������Ϣת��Ϊ JSON �ַ�������д����Ӧ�塣
        // ���� true����ʾ������ɡ�
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        if (!src_root.isMember("email")) {
            std::cout<< "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }
        // ��ȡ JSON �����е� email �ֶβ������
        // ������Ӧ�еĴ�����Ϊ 0����ʾ�ɹ���
        // �� email �ֶ�Ҳ��������Ӧ JSON �С�
        // ����Ӧ JSON ת��Ϊ�ַ�������д����Ӧ�塣
        // ���� true����ʾ������ɡ�
        auto email = src_root["email"].asString();
        GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        std::cout << "email is " << email << std::endl;
        root["error"] = rsp.error();
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
        });

    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();
        auto confirm = src_root["confirm"].asString();
        auto icon = src_root["icon"].asString();

        if (pwd != confirm) {
            std::cout << "password err " << std::endl;
            root["error"] = ErrorCodes::PasswdErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        //�Ȳ���redis��email��Ӧ����֤���Ƿ����
        std::string  varify_code;
        bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX+src_root["email"].asString(), varify_code);
        if (!b_get_varify) {
            std::cout << " get varify code expired" << std::endl;
            root["error"] = ErrorCodes::VarifyExpired;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        if (varify_code != src_root["varifycode"].asString()) {
            std::cout << " varify code error" << std::endl;
            root["error"] = ErrorCodes::VarifyCodeErr;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }


        //�������ݿ��ж��û��Ƿ����

        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = pwd;
        root["confirm"] = confirm;
        root["varifycode"] = src_root["varifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return true;
        });
}


/*
������path�������·����
      con��ָ�� HttpConnection ����Ĺ���ָ�롣
���ȼ�� _get_handlers ӳ�����Ƿ���� path ��Ӧ�Ĵ��������������ڣ����� false ��ʾ����ʧ�ܡ�
������ڣ����ö�Ӧ�Ĵ�����򣬲����� con ������
���óɹ��󣬷��� true ��ʾ����ɹ���
*/
bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }

    _get_handlers[path](con);
    return true;
}


bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_post_handlers.find(path) == _post_handlers.end()) {
        return false;
    }

    _post_handlers[path](con);
    return true;
}