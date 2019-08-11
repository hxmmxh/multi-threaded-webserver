//解析完毕后的请求报文

#ifndef HXMMXH_HTTPREQUEST_H
#define HXMMXH_HTTPREQUEST_H

#include "../Time/Timestamp.h"

#include <map>
#include <string>

namespace hxmmxh
{

class HttpRequest
{
public:
    enum HttpMethod
    {
        Invalid,
        Get,
        Post,
        Head
    };
    enum HttpVersion
    {
        Unknown,
        Http10,
        Http11
    };
    HttpRequest()
        : method_(Invalid),
          version_(Unknown)
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
    const char *versionString() const
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

    bool setMethod(const char *start, const char *end)
    {
        std::string m(start, end);
        if (m == "GET")
        {
            method_ = Get;
        }
        else if (m == "POST")
        {
            method_ = Post;
        }
        else if (m == "HEAD")
        {
            method_ = Head;
        }
        else
        {
            method_ = Invalid;
        }
        return method_ != Invalid;
    }
    HttpMethod method() const
    {
        return method_;
    }
    const char *methodString() const
    {
        const char *result = "UNKNOWN";
        switch (method_)
        {
        case Get:
            result = "GET";
            break;
        case Post:
            result = "POST";
            break;
        case Head:
            result = "HEAD";
            break;
        default:
            break;
        }
        return result;
    }


    void setPath(const char *start, const char *end)
    {
        path_.assign(start, end);
        size_t slash = path_.find_last_of('/');
        if(slash==std::string::npos)
            filename_ = path_;
        else
            filename_ = path_.substr(slash + 1);
    }
    const std::string &path() const
    {
        return path_;
    }
    const std::string &filename() const
    {
        return filename_;
    }
    void setQuery(const char *start, const char *end)
    {
        query_.assign(start, end);
    }
    const std::string &query() const
    {
        return query_;
    }

    void setReceiveTime(Timestamp t)
    {
        receiveTime_ = t;
    }
    Timestamp receiveTime() const
    {
        return receiveTime_;
    }

    //格式为 首部字段名：字段值
    //例如 Content_Type: text/html
    //colon:冒号
    //end指向'/r'
    void addHeader(const char *start, const char *colon, const char *end)
    {
        std::string field(start, colon);
        ++colon;
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }
        std::string value(colon, end);
        while (!value.empty() && isspace(value[value.size() - 1]))
        {
            value.resize(value.size() - 1);
        }
        headers_[field] = value;
    }

    //查找首部字段的值
    std::string getHeader(const std::string &field) const
    {
        std::string result;
        std::map<std::string, std::string>::const_iterator it = headers_.find(field);
        if (it != headers_.end())
        {
            result = it->second;
        }
        return result;
    }

    const std::map<std::string, std::string> &headers() const
    {
        return headers_;
    }

private:
    HttpMethod method_;
    HttpVersion version_;
    //URI中包含文件路径和？后面的查询字符串
    std::string path_;
    std::string filename_;
    std::string query_;
    Timestamp receiveTime_;
    //首部字段，由首部字段名和字段值构成
    std::map<std::string, std::string> headers_;
};
} // namespace hxmmxh

#endif