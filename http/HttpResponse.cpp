#include "HttpResponse.h"
#include "../WebServer/Buffer.h"

#include <stdio.h>

using namespace hxmmxh;

const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms

//往缓冲区中写入返回报文
//版本+空格+状态码+空格+状态码描述+回车+换行
//头部字段名+':'+字段值+回车+换行
//...
//头部字段名+':'+字段值+回车+换行
//回车+换行
//报文主体
void HttpResponse::appendToBuffer(Buffer *output) const
{
    char buf[32];
    snprintf(buf, sizeof buf, "%s %d ", versionstring(), statusCode_);
    //回车/r,换行/n
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");
    //头部字段名+':'+字段值+回车+换行
    //短连接还是长连接
    if (closeConnection_)
    {
        output->append("Connection: close\r\n");
    }
    else
    {
        output->append("Connection: Keep-Alive\r\n");
        output->append("Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n");
    }

    for (const auto &header : headers_)
    {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }
    output->append("\r\n");
    output->append(body_);
}