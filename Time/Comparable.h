#ifndef HXMMXH_COMPARABLE_H
#define HXMMXH_COMPARABLE_H


namespace hxmmxh
{
//equality_comparable：要求提供==，可自动实现!=;
//less_than_comparable：要求提供<，可自动实现>、<=、>=;
template <class T>
class equality_comparable_hxmmxh
{
    friend bool operator!=(const T &lhs, const T &rhs)
    {
        return !(lhs == rhs);
    }
};

template <class T>
class less_than_comparable_hxmmxh
{
    friend bool operator>=(const T &lhs, const T &rhs)
    {
        return !(lhs < rhs);
    }
    friend bool operator>(const T &lhs, const T &rhs)
    {
        return rhs < lhs;
    }
    friend bool operator<=(const T &lhs, const T &rhs)
    {
        return !(lhs > rhs);
    }
};
}

#endif