#ifndef HXMMXH_HTTPPARSE_H
#define HXMMXH_HTTPPARSE_H

#include "HttpRequest.h"

namespace hxmmxh
{
class HttpParse
{
public:
    enum HttpRequestParseState
    {
        ExpectRequestLine, //等待请求行
        ExpectHeaders,     //等待头部字段
        ExpectEmptyline,   //等待空行
        ExpectBody,        //等待主体
        Success,           //解析完毕
    };
    HttpParse()
        : state_(kExpectRequestLine)
    {
    }
    //开始解析buf里的Http报文，出错返回flase,receiveTime为收到可读信号的时间
    bool parseRequest(Buffer *buf, Timestamp receiveTime);
    bool success() const
    {
        return state_ == Success;
    }
    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }
    const HttpRequest &request() const
    {
        return request_;
    }

    HttpRequest &request()
    {
        return request_;
    }

private:
    //解析请求行
    bool processRequestLine(const char *begin, const char *end);
    HttpRequestParseState state_;
    HttpRequest request_;
};
} // namespace hxmmxh

#endif