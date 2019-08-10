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
        kExpectRequestLine, //等待请求行
        kExpectHeaders,     //等待头部
        kExpectBody,        //等待主体
        kGotAll,            //解析完毕
    };
    HttpParse()
        : state_(kExpectRequestLine)
    {
    }
    //开始解析，出错返回flase
    bool parseRequest(Buffer *buf, Timestamp receiveTime);
    bool gotAll() const
    {
        return state_ == kGotAll;
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
    bool processRequestLine(const char *begin, const char *end);

    HttpRequestParseState state_;
    HttpRequest request_;
};
} // namespace hxmmxh

#endif