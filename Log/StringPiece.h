/*
目的是为了实现了高效的字符串传递，节省string的拷贝开销
1. 统一了参数格式，不需要为const char* const std::string&等分别实现功能相同的函数了，参数统一指定为StringPiece即可
2. 节约了数据拷贝以及strlen的调用
3. 没有字符串的控制权，不负责构造以及销毁。调用者需要保证在BasicStringPiece的生命周期里源buffer始终有效
 */

#ifndef HXMMXH_STRINGPIECE_H
#define HXMMXH_STRINGPIECE_H

#include <cstring> //memcmp
#include <string>
//input output stream forward
//输入输出流前向声明。 提供所有输入输出类的声明。
#include <iosfwd>

namespace hxmmxh
{
class StringArg
{
public:
    StringArg(const char *str) : str_(str) {}
    StringArg(const std::string &str) : str_(str.c_str()) {}

    const char *c_str() const { return str_; }

private:
    const char *str_;
};

class StringPiece
{
public:
    //非explict的构造函数，使得const char*和string能隐式转化成StringPiece
    StringPiece()
        : ptr_(NULL), length_(0) {}
    StringPiece(const char *str)
        : ptr_(str), length_(static_cast<int>(strlen(ptr_))) {}
    StringPiece(const unsigned char *str)
        : ptr_(reinterpret_cast<const char *>(str)),
          length_(static_cast<int>(strlen(ptr_))) {}
    StringPiece(const std::string &str)
        : ptr_(str.data()), length_(static_cast<int>(str.size())) {}
    StringPiece(const char *offset, int len)
        : ptr_(offset), length_(len) {}

    //返回的字符指针可能不以‘\0'结尾
    const char *data() const { return ptr_; }
    int size() const { return length_; }
    bool empty() const { return length_ == 0; }
    const char *begin() const { return ptr_; }
    const char *end() const { return ptr_ + length_; }
    //修改操作
    void clear()
    {
        ptr_ = NULL;
        length_ = 0;
    }
    void set(const char *buffer, int len)
    {
        ptr_ = buffer;
        length_ = len;
    }
    void set(const char *str)
    {
        ptr_ = str;
        length_ = static_cast<int>(strlen(str));
    }
    void set(const void *buffer, int len)
    {
        ptr_ = reinterpret_cast<const char *>(buffer);
        length_ = len;
    }
    //下标运算符
    char operator[](int i) const { return ptr_[i]; }
    //由于stringPiece不具有字符串的控制权，所以删除只是移动游标和删减长度
    //删除开头n个字符
    void remove_prefix(int n)
    {
        ptr_ += n;
        length_ -= n;
    }
    //删除结尾n个字符
    void remove_suffix(int n)
    {
        length_ -= n;
    }
    //重载比较运算符
    bool operator==(const StringPiece &x) const
    {
        return ((length_ == x.length_) &&
                (memcmp(ptr_, x.ptr_, length_) == 0));
    }
    bool operator!=(const StringPiece &x) const
    {
        return !(*this == x);
    }
//定义一个宏简化stringpiece的比较操作
#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                                \
    bool operator cmp(const StringPiece &x) const                                \
    {                                                                            \
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
        return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
    }
    STRINGPIECE_BINARY_PREDICATE(<, <);
    STRINGPIECE_BINARY_PREDICATE(<=, <);
    STRINGPIECE_BINARY_PREDICATE(>=, >);
    STRINGPIECE_BINARY_PREDICATE(>, >);
#undef STRINGPIECE_BINARY_PREDICATE

    int compare(const StringPiece &x) const
    {
        int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
        if (r == 0)
        {
            if (length_ < x.length_)
                r = -1;
            else if (length_ > x.length_)
                r = +1;
        }
        return r;
    }

    std::string as_string() const
    {
        return std::string(data(), size());
    }

    void CopyToString(std::string *target) const
    {
        target->assign(ptr_, length_);
    }

    // 判断是否以x起始
    bool starts_with(const StringPiece &x) const
    {
        return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
    }

private:
    const char *ptr_;
    int length_;
};

} // namespace hxmmxh

// allow StringPiece to be logged
std::ostream &operator<<(std::ostream &o, const hxmmxh::StringPiece &piece);

#endif