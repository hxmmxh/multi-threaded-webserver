#include "Buffer.h"

#include <errno.h>
#include <memory.h>
#include <sys/uio.h> //iovec，readv

using namespace hxmmxh;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof(extrabuf);
  const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);
  if (n < 0) {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    //返回值大于 writable
    //说明把buffer写满了，再把extrabuf里的数据append到buffer中
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}
