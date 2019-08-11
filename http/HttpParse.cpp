#include "HttpParse.h"
#include "../WebServer/Buffer.h"

#include <string>
#include <map>

using namespace hxmmxh;

bool HttpParse::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    //setMethod中如果方法不是HEAD,GET POST三种会返回错误
    //用一行就完成了方法的解析
    if (space != end && request_.setMethod(start, space))
    {
        //开始解析URI
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end)
        {
            const char *question = std::find(start, space, '?');
            if (question != space)
            {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            else
            {
                request_.setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed)
            {
                if (*(end - 1) == '1')
                {
                    request_.setVersion(HttpRequest::Http11);
                }
                else if (*(end - 1) == '0')
                {
                    request_.setVersion(HttpRequest::Http10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

bool HttpParse::parseRequest(Buffer *buf, Timestamp receiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore && ok)
    {
        //需要解析请求行
        if (state_ == ExpectRequestLine)
        {
            const char *crlf = buf->findCRLF();
            if (crlf)
            {
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    //buf里字符指针往后移动到下一行
                    buf->retrieveUntil(crlf + 2);
                    state_ = ExpectHeaders;
                    if (buf->readableBytes() == 0)
                    {
                        hasMore = false;
                        state_ = Success;
                    }
                }
            }
            else
            {
                ok = false;
                hasMore = false;
            }
        }
        //需要解析头部
        else if (state_ == ExpectHeaders)
        {
            const char *crlf = buf->findCRLF();
            if (crlf)
            {
                const char *colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                {
                    request_.addHeader(buf->peek(), colon, crlf);
                    buf->retrieveUntil(crlf + 2);
                    if (buf->readableBytes() == 0)
                    {
                        hasMore = false;
                        state_ = Success;
                    }
                }
                //没有找到冒号，说明这行不包含首部字段，去判断它是否为空行
                else
                {
                    state_ = ExpectEmptyline;
                }
            }
            else
            {
                ok = false;
                hasMore = false;
            }
        }
        //解析空行
        else if (state_ == ExpectEmptyline)
        {
            const char *crlf = buf->findCRLF();
            if (crlf)
            {
                if (crlf == buf->peek())
                {
                    state_ = ExpectBody;
                    buf->retrieveUntil(crlf + 2);
                    if (buf->readableBytes() == 0)
                    {
                        hasMore = false;
                        state_ = Success;
                    }
                }
                else
                {
                    hasMore = false;
                    ok = false;
                }
            }
            else
            {
                ok = false;
                hasMore = false;
            }
        }
        //实际上测试用的请求报文都没有主体
        else if (state_ == ExpectBody)
        {
            state_ = Success;
            hasMore = false;
        }
    }
    return ok;
}
