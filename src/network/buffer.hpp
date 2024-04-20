#pragma once

#include <string.h>

#include <vector>
#include <string>

namespace reactor {

class ArrayBuffer
{
public:
    static const size_t PACKETSIZE = 40960;
    static const size_t PREPEND = 0;

public:
    ArrayBuffer(size_t initSize = PACKETSIZE) noexcept;
    ArrayBuffer(char const * ptr, size_t len, size_t initSize = PACKETSIZE);
    ArrayBuffer(const ArrayBuffer &) = default;
    ArrayBuffer(ArrayBuffer &&) = default;
    ArrayBuffer & operator=(const ArrayBuffer &) = default;
    ArrayBuffer & operator=(ArrayBuffer &&) = default;

    ~ArrayBuffer() = default;

    void swap(ArrayBuffer & buf);

    size_t readableBytes() const noexcept;

    size_t writableBytes() const noexcept;

private:
    char * const begin() noexcept;

    char const * const begin() const noexcept;

public:
    char * beginWrite() noexcept;

    char const * beginWrite() const noexcept;

    char * const peek() noexcept;

    char const * peek() const noexcept;

    char const * findEOL() const;

    char const * findEOL(char const * start);

    void retrieve(size_t len);

    void retrieveUntil(char const * end);

    void retrieveAll() noexcept;

    std::string retrieveAllAsString();

    std::string retrieveAsString(size_t len);

    void retrieveInt64();

    void retrieveInt32();

    void retrieveInt16();

    void retrieveInt8();

    void append(char const * data, size_t len);

    void append(void const* data, size_t len);

    void ensureWritableBytes(size_t len);

    void hasWritten(size_t len);

private:
    size_t prependableBytes() const;

    void makeSpace(size_t len);

private:
    size_t              readerIndex_;
    size_t              writerIndex_;
    std::vector<char>   buffer_;
};

} // namespace reactor