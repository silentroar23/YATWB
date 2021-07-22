#pragma once

#include <assert.h>

#include <algorithm>
#include <string>
#include <vector>
//#include <unistd.h>  // ssize_t

/**
 * A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
 *
 * @code
 * +-------------------+------------------+------------------+
 * | prependable bytes |  readable bytes  |  writable bytes  |
 * |                   |     (CONTENT)    |                  |
 * +-------------------+------------------+------------------+
 * |                   |                  |                  |
 * 0      <=      readerIndex   <=   writerIndex    <=     size
 * @endcode
 */

class Buffer {
 public:
  static const size_t CheapPrependSize = 8;
  static const size_t InitialSize = 1024;

  Buffer()
      : buffer_(CheapPrependSize + InitialSize),
        reader_idx_(CheapPrependSize),
        writer_idx_(CheapPrependSize) {
    assert(readableBytes() == 0);
    assert(writableBytes() == InitialSize);
    assert(prependableBytes() == CheapPrependSize);
  }

  /* default copy-ctor, dtor and assignment are fine */

  void swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(reader_idx_, rhs.reader_idx_);
    std::swap(writer_idx_, rhs.writer_idx_);
  }

  size_t readableBytes() const { return writer_idx_ - reader_idx_; }

  size_t writableBytes() const { return buffer_.size() - writer_idx_; }

  size_t prependableBytes() const { return reader_idx_; }

  /**
   * @return: the first readable char
   */
  const char* peek() const { return begin() + reader_idx_; }

  /**
   * Application reads from application buffer
   * After read, move reader_idx_ right by @len
   */
  void retrieve(size_t len) {
    assert(len <= readableBytes());
    reader_idx_ += len;
  }

  /**
   * Retrieve [peek(), end)
   * Note: does not retrive end
   */
  void retrieveUntil(const char* end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  /**
   * Retrive [peek(), beginWrite())
   * Set buffer_ to initial state
   */
  void retrieveAll() {
    reader_idx_ = CheapPrependSize;
    writer_idx_ = CheapPrependSize;
  }

  std::string retrieveAsString() {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
  }

  /**
   * Append data after beginWrite()
   * Make space if need be
   */
  void append(const char* /*restrict*/ data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  void append(const std::string& str) { append(str.data(), str.length()); }

  void append(const void* /*restrict*/ data, size_t len) {
    append(static_cast<const char*>(data), len);
  }

  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char* beginWrite() { return begin() + writer_idx_; }

  const char* beginWrite() const { return begin() + writer_idx_; }

  void hasWritten(size_t len) { writer_idx_ += len; }

  void prepend(const void* /*restrict*/ data, size_t len) {
    assert(len <= prependableBytes());
    reader_idx_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + reader_idx_);
  }

  void shrink(size_t reserve) {
    std::vector<char> buf(CheapPrependSize + readableBytes() + reserve);
    std::copy(peek(), peek() + readableBytes(), buf.begin() + CheapPrependSize);
    buf.swap(buffer_);
  }

  /**
   * Read data directly into buffer.
   *
   * It may implement with readv(2)
   * @return result of read(2), @c errno is saved
   */
  ssize_t readFd(int fd, int* saved_errno);

 private:
  /* The beginning position of the underlying buffer_ */
  char* begin() { return &*buffer_.begin(); }

  const char* begin() const { return &*buffer_.begin(); }

  void makeSpace(size_t len) {
    /**
     * Current capacity is not big enough, need to resize the underlying buffer_
     */
    if (writableBytes() + prependableBytes() < len + CheapPrependSize) {
      buffer_.resize(writer_idx_ + len);
    } else {
      /**
       * Move reader_idx_ back to the initial position
       *
       * Move readable data to the front, make space inside buffer
       */
      assert(CheapPrependSize < reader_idx_);
      size_t readable = readableBytes();
      std::copy(begin() + reader_idx_, begin() + writer_idx_,
                begin() + CheapPrependSize);
      reader_idx_ = CheapPrependSize;
      writer_idx_ = reader_idx_ + readable;
      assert(readable == readableBytes());
    }
  }

 private:
  std::vector<char> buffer_;
  size_t reader_idx_;
  size_t writer_idx_;
};
