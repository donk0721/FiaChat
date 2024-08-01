#include "HttpConnection.h"
#include <iostream>
#include "LogicSystem.h"

HttpConnection::HttpConnection(boost::asio::io_context& ioc): _socket(ioc) {
}

void HttpConnection::Start()
{
    auto self = shared_from_this();
    http::async_read(_socket, _buffer, _request, [self](beast::error_code ec,
        std::size_t bytes_transferred) {
            try {
                if (ec) {
                    std::cout << "http read err is " << ec.what() << std::endl;
                    return;
                }

                //�������������
                boost::ignore_unused(bytes_transferred);
                self->HandleReq();
                self->CheckDeadline();
            }
            catch (std::exception& exp) {
                std::cout << "exception is " << exp.what() << std::endl;
            }
        }
    );
}


//char תΪ16����
unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

//Url���뺯��
std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //�ж��Ƿ�������ֺ���ĸ����
        //����ַ�����ĸ�����֡�-��_��. �� ~��ֱ����ӵ� strTemp ��
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //Ϊ���ַ�
            strTemp += "+";
        else
        {
            //�����ַ���Ҫ��ǰ��%���Ҹ���λ�͵���λ�ֱ�תΪ16����
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}

//Url���뺯��
std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //��ԭ+Ϊ��
        if (str[i] == '+') strTemp += ' ';
        //����%������������ַ���16����תΪchar��ƴ��
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

//����Url�еĲ���
void HttpConnection::PreParseGetParam() {
    // ��ȡ URI  
    auto uri = _request.target();
    // ���Ҳ�ѯ�ַ����Ŀ�ʼλ�ã��� '?' ��λ�ã�  
    auto query_pos = uri.find('?');
    // û���ʺţ�˵���ǲ�������������
    if (query_pos == std::string::npos) {
        _get_url = uri;
        return;
    }

    // get_url�����˴�0��?֮ǰ�ĵ�λ��
    _get_url = uri.substr(0, query_pos);

    // query_string�ӣ���һλ��ʼ���ַ�����β
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;

    // ѭ����'&'��&֮ǰ�ľ���key=value����ʽ
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos)); // ��ȡkeyȻ�����  
            value = UrlDecode(pair.substr(eq_pos + 1)); // ��ȡvalueȻ�����
            _get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    // �������һ�������ԣ����û�� & �ָ�����  
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            _get_params[key] = value;
        }
    }
}

void HttpConnection::HandleReq() {
    // ������Ӧ�� HTTP �汾
    _response.version(_request.version());

    // ��������Ϊ�����ӣ�������Ӧ��ر����ӣ�
    _response.keep_alive(false);

    // ���� GET ����
    if (_request.method() == http::verb::get) {
        // ʹ�� LogicSystem ���� GET ����
        PreParseGetParam();
        bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());

        // �������ʧ�ܣ�������Ӧ״̬Ϊ 404 Not Found�� 
        // ������Ӧͷ�е� Content-Type �ֶ�Ϊ text/plain�� 
        // ʹ�� beast::ostream ����Ӧ����д�� "url not found\r\n"�� 
        // ���� WriteResponse ����������Ӧ��
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        // �������ɹ������� 200 OK ��Ӧ
        // ������Ӧͷ�е� Server �ֶ�Ϊ "GateServer"
        // ���� WriteResponse ����������Ӧ
        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
    //����POST����
    if (_request.method() == http::verb::post) {
        bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());

        //�������ʧ��
        if (!success) {
            // �������ʧ�ܣ�������Ӧ״̬Ϊ 404 Not Found�� 
            // ������Ӧͷ�е� Content-Type �ֶ�Ϊ text/plain�� 
            // ʹ�� beast::ostream ����Ӧ����д�� "url not found\r\n"�� 
            // ���� WriteResponse ����������Ӧ��
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        // �������ɹ������� 200 OK ��Ӧ
        // ������Ӧͷ�е� Server �ֶ�Ϊ "GateServer"
        // ���� WriteResponse ����������Ӧ
        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}


void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    //���� HTTP ��Ӧͷ�е� Content-Length �ֶ�Ϊ��Ӧ��Ĵ�С�������ͻ��˾�֪��������Ҫ��ȡ����Ӧ��ĳ���
    _response.content_length(_response.body().size());

    /*
    http::async_write ��һ���첽д���������ڽ� HTTP ��Ӧ��Ϣ���͵��ͻ���
    _socket�����ڷ������ݵ� TCP �׽���
    _response��Ҫ���͵� HTTP ��Ӧ��Ϣ
    ������������һ�� lambda �ص���������д������ɺ󱻵���
    beast::error_code ec�������룬���д������з��������� ec �������صĴ�����Ϣ
    std::size_t����д����ֽ���
    */
    http::async_write(_socket,_response,
        [self](beast::error_code ec, std::size_t)
        {
            //�ر��׽��ֵķ��ͷ���֪ͨ�ͻ���û�и��������Ҫ���͡���ʹ���� TCP �׽��ֵ� shutdown_send ѡ��
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            //ȡ����ʱ����ֹͣ�κ����������صĳ�ʱ����
            self->deadline_.cancel();
        });
}

//������ӵĳ�ʱʱ�䣬���ڳ�ʱ��ر��׽���
void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();

    deadline_.async_wait(
        [self](beast::error_code ec)
        {
            if (!ec)
            {
                // Close socket to cancel any outstanding operation.
                self->_socket.close(ec);
            }
        });
} 