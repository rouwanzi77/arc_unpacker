#include "types.h"
#include <algorithm>

using namespace au;

const size_t bstr::npos = static_cast<size_t>(-1);

bstr::bstr()
{
}

bstr::bstr(const size_t n, u8 fill) : v(n, fill)
{
}

bstr::bstr(const u8 *str, const size_t size) : v(str, str + size)
{
}

bstr::bstr(const char *str, const size_t size) : v(str, str + size)
{
}

bstr::bstr(const std::string &other) : v(other.begin(), other.end())
{
}

std::string bstr::str(bool trim_to_zero) const
{
    if (trim_to_zero)
        return std::string(get<char>());
    return std::string(get<char>(), size());
}

bool bstr::empty() const
{
    return v.size() == 0;
}

size_t bstr::size() const
{
    return v.size();
}

size_t bstr::capacity() const
{
    return v.capacity();
}

size_t bstr::find(const bstr &other) const
{
    const auto pos = std::search(
        v.begin(), v.end(),
        other.get<u8>(), other.get<u8>() + other.size());
    if (pos == v.end())
        return bstr::npos;
    return pos - v.begin();
}

bstr bstr::substr(const size_t start) const
{
    if (start > size())
        return ""_b;
    return bstr(get<const u8>() + start, size() - start);
}

bstr bstr::substr(const size_t start, const size_t size) const
{
    if (start > v.size())
        return ""_b;
    if (size > v.size() || start + size > v.size())
        return substr(start, v.size() - start);
    return bstr(get<const u8>() + start, size);
}

void bstr::resize(const size_t how_much)
{
    v.resize(how_much);
}

void bstr::reserve(const size_t how_much)
{
    v.reserve(how_much);
}

bstr bstr::operator +(const bstr &other) const
{
    bstr ret(&v[0], size());
    ret += other;
    return ret;
}

void bstr::operator +=(const bstr &other)
{
    v.insert(v.end(), other.get<u8>(), other.get<u8>() + other.size());
}

void bstr::operator +=(const char c)
{
    v.push_back(c);
}

void bstr::operator +=(const u8 c)
{
    v.push_back(c);
}

bool bstr::operator ==(const bstr &other) const
{
    return v == other.v;
}

bool bstr::operator !=(const bstr &other) const
{
    return v != other.v;
}

u8 &bstr::operator [](const size_t pos)
{
    return v[pos];
}

const u8 &bstr::operator [](const size_t pos) const
{
    return v[pos];
}

u8 &bstr::at(const size_t pos)
{
    return v.at(pos);
}

const u8 &bstr::at(const size_t pos) const
{
    return v.at(pos);
}
