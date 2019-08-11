//响应报文
#ifndef HXMMXH_HTTPRESPONSE_H
#define HXMMXH_HTTPRESPONSE_H

#include <string>
#include <map>

namespace hxmmxh
{
class Buffer;
class HttpResponse
{
public:
    enum HttpStatusCode
    {
        SUnknown,
        S200Ok = 200,
        S301MovedPermanently = 301,
        S400BadRequest = 400,
        S404NotFound = 404,
    };
    enum HttpVersion
    {
        VUnknown,
        Http10,
        Http11
    };

    explicit HttpResponse(bool close)
        : statusCode_(SUnknown),
          version_(VUnknown),
          closeConnection_(close)
    {
    }

    void setVersion(HttpVersion v)
    {
        version_ = v;
    }
    HttpVersion getVersion() const
    {
        return version_;
    }
    const char *versionstring() const
    {
        const char *result = "UNKNOWN";
        switch (version_)
        {
        case Http10:
            result = "HTTP/1.0";
            break;
        case Http11:
            result = "HTTP/1.1";
        default:
            break;
        }
        return result;
    }

    void setStatusCode(HttpStatusCode code)
    {
        statusCode_ = code;
    }
    void setStatusMessage(const std::string &message)
    {
        statusMessage_ = message;
    }

    void setCloseConnection(bool on)
    {
        closeConnection_ = on;
    }

    bool closeConnection() const
    {
        return closeConnection_;
    }

    void setContentType(const std::string &contentType)
    {
        addHeader("Content-Type", contentType);
    }
    void addHeader(const std::string &key, const std::string &value)
    {
        headers_[key] = value;
    }

    void setBody(const std::string &body)
    {
        body_ = body;
        addHeader("Content-Length", std::to_string(body.size()));
    }
    //把响应报文格式化写入Buffer中
    void appendToBuffer(Buffer *output) const;

private:
    //是否要在发送报文后关闭连接
    bool closeConnection_;
    //版本+空格+状态码+空格+状态码描述+回车+换行
    HttpVersion version_;
    HttpStatusCode statusCode_;
    std::string statusMessage_;
    //头部字段
    std::map<std::string, std::string> headers_;
    //报文主体
    std::string body_;
};
} // namespace hxmmxh
#endif