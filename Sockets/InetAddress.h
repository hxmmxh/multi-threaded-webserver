#ifndef HXMMXH_INETADDRESS_H
#define HXMMXH_INETADDRESS_H

#include "StringPiece.h"

#include <endian.h>
#include <netinet/in.h>
#include <string>

namespace hxmmxh
{
namespace sockets
{

//网络字节序是大端be
//h代表主机字节序
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
inline uint64_t hostToNetwork64(uint64_t host64)
{
    return htobe64(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
    return htobe32(host32);
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
    return htobe16(host16);
}

inline uint64_t networkToHost64(uint64_t net64)
{
    return be64toh(net64);
}

inline uint32_t networkToHost32(uint32_t net32)
{
    return be32toh(net32);
}

inline uint16_t networkToHost16(uint16_t net16)
{
    return be16toh(net16);
}

#pragma GCC diagnostic pop
}

class InetAddress 
{
public:
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
    InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);
    explicit InetAddress(const struct sockaddr_in &addr) : addr_(addr) {}
    explicit InetAddress(const struct sockaddr_in6 &addr) : addr6_(addr) {}

    sa_family_t family() const { return addr_.sin_family; }
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    uint32_t ipNetEndian() const;
    uint16_t portNetEndian() const { return addr_.sin_port; }

    const struct sockaddr *getSockAddr() const { return static_cast<const struct sockaddr *>((void*)&addr6_); }
    void setSockAddrInet6(const struct sockaddr_in6 &addr6) { addr6_ = addr6; }

    static bool resolve(StringArg hostname, InetAddress *result);

    void setScopeId(uint32_t scope_id);

private:
    union {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
};
} // namespace hxmmxh

#endif