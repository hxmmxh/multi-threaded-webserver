#include "EventLoop.h"
#include "TcpServer.h"

#include <string>
#include <map>
#include <iostream>

using namespace hxmmxh;
using namespace std;

typedef std::map<string, string> UserMap;
UserMap users;

string getUser(const string& user)
{
  string result = "No such user";
  UserMap::iterator it = users.find(user);
  if (it != users.end())
  {
    result = it->second;
  }
  return result;
}

void onMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               Timestamp receiveTime)
{
  string data(buf->peek(), buf->readableBytes());
  cout << data;
  const char *crlf = buf->findCRLF();
  if (crlf)
  {
    string user(buf->peek(), crlf);
    conn->send(getUser(user) + "\r\n");
    buf->retrieveUntil(crlf + 2);
    conn->shutdown();
  }
}

int main()
{
  users["schen"] = "Happy and well";
  EventLoop loop;
  TcpServer server(&loop, InetAddress(1080), "Finger");
  server.setMessageCallback(onMessage);
  server.start();
  loop.loop();
}
