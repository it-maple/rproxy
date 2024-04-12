#include <assert.h>

#include <stdexcept>

#include "buffer.hpp"

namespace reactor {

ArrayBuffer::ArrayBuffer(size_t initSize) noexcept
    : readerIndex_(PREPEND)
    , writerIndex_(PREPEND)
    , buffer_(initSize)
    {}

ArrayBuffer::ArrayBuffer(char const * ptr, size_t len, size_t initSize)
    : readerIndex_(PREPEND)
    , writerIndex_(PREPEND)
    , buffer_(initSize)
{
    if (!ptr)
        throw std::logic_error("pointer is null.");
    if (len > INITSIZE)
        throw std::logic_error("buffer size is not enough.");

    ::memcpy(buffer_.data(), ptr, len);
    writerIndex_ += len;
}

void ArrayBuffer::swap(ArrayBuffer & buf)
{
    buffer_.swap(buf.buffer_);
    std::swap(readerIndex_, buf.readerIndex_);
    std::swap(writerIndex_, buf.writerIndex_);
}

size_t ArrayBuffer::readableBytes() const noexcept
{
    return writerIndex_ - readerIndex_;
}

size_t ArrayBuffer::writableBytes() const noexcept
{
    return buffer_.size() - writerIndex_;
}

char * const ArrayBuffer::begin() noexcept
{
    return &*buffer_.begin();
}

char const * const ArrayBuffer::begin() const noexcept
{
    return &*buffer_.begin();
}

char * ArrayBuffer::beginWrite() noexcept
{
    return begin() + writerIndex_;
}

char const * ArrayBuffer::beginWrite() const noexcept
{
    return begin() + writerIndex_;
}

char * const ArrayBuffer::peek() noexcept
{
    return begin() + readerIndex_;
}

char const * ArrayBuffer::peek() const noexcept
{
    return begin() + readerIndex_;
}

char const * ArrayBuffer::findEOL() const
{
    void const * eol = ::memchr(peek(), '\n', readableBytes());
    return static_cast<char const *>(eol);
}

char const * ArrayBuffer::findEOL(char const * start)
{
    assert(peek() <= start);
    assert(start <= beginWrite());
    void const * eol = ::memchr(start, '\n', beginWrite() - start);

    return static_cast<char const *>(eol);
}

void ArrayBuffer::retrieve(size_t len)
{
    if (len > readableBytes())
        throw std::logic_error("no such size for buffer");

    if (len < readableBytes())
        readerIndex_ += len;
    else
        retrieveAll();
}

void ArrayBuffer::retrieveUntil(char const * end)
{
    if (peek() > end || end > beginWrite())
        throw std::logic_error("invalid position.");

    retrieve(end - peek());
}

void ArrayBuffer::retrieveAll() noexcept
{
    readerIndex_ = PREPEND;
    writerIndex_ = PREPEND;
}

std::string ArrayBuffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
}

std::string ArrayBuffer::retrieveAsString(size_t len)
{
    if (len > readableBytes())
        throw std::logic_error("no such size for buffer");

    std::string result(peek(), len);
    retrieve(len);

    return result;
}

void ArrayBuffer::retrieveInt64() { retrieve(sizeof(int64_t)); }

void ArrayBuffer::retrieveInt32() { retrieve(sizeof(int32_t)); }

void ArrayBuffer::retrieveInt16() { retrieve(sizeof(int16_t)); }

void ArrayBuffer::retrieveInt8() { retrieve(sizeof(int8_t)); }

void ArrayBuffer::append(char const * data, size_t len)
{
    if (!data)
        throw std::logic_error("invalid pointer.");

    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}

void ArrayBuffer::append(void const * data, size_t len)
{
    append(static_cast<char const*>(data), len);
}

void ArrayBuffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() < len)
    {
      makeSpace(len);
    }
}

inline void ArrayBuffer::hasWritten(size_t len)
{
    if (len > writableBytes())
        throw std::logic_error("the expected length gather than the exact.");

    writerIndex_ += len;
}

void ArrayBuffer::makeSpace(size_t len)
{
    if (writableBytes() + prependableBytes() < len + PREPEND)
    {
      buffer_.resize(writerIndex_ + len);
    }
    else
    {
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_,
                 begin() + writerIndex_,
               begin() + PREPEND);
      readerIndex_ = PREPEND;
      writerIndex_ = readerIndex_ + readable;

      if (readable != readableBytes())
        throw std::logic_error("the readable size is not equal to the size after resize-ing buffer.");
    }
}

inline size_t ArrayBuffer::prependableBytes() const
{
    return readerIndex_;
}

} // namespace reactor