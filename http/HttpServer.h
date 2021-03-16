#ifndef HXMMXH_HTTPSERVER_H
#define HXMMXH_HTTPSERVER_H

#include "TcpServer.h"

#include <string>

namespace hxmmxh
{
class HttpRequest;
class HttpResponse;

class HttpServer
{
public:
    HttpServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const std::string &name,
               TcpServer::Option option = TcpServer::kNoReusePort);
    EventLoop *getLoop() const { return server_.getLoop(); }
    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }
    void start();

private:
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp receiveTime);
    void ReplyRequest(const TcpConnectionPtr &, const HttpRequest );   
    void WriteResponse(const HttpRequest&, HttpResponse*);

    TcpServer server_;
};
} // namespace hxmmxh

#endif