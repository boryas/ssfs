#pragma once

#include <memory>
#include <string>
#include <stdexcept>

namespace ssfs {
namespace string {

class StringViewError : public std::runtime_error {
 public:
  StringViewError(const std::string& reason)
    : std::runtime_error("StringViewError: " + reason) {}
};

/*
 * StringView facilitates minimizing string copies while doing "read-only"
 * work like processing requests. We guarantee that the string we are
 * referring to exists for the duration of its use by storing it in a
 * shared_ptr, while adding some additional book-keeping for zero-copy
 * substring functionality.
 *
 * Until there is a working "StringViewChain" we suffer the indignity
 * of copying in operator+ and operator+= to support necessary functionality
 * in the parser.
 */
class StringView {
 public:
  StringView();
  StringView(std::shared_ptr<std::string> str);
  StringView(std::shared_ptr<std::string> str, size_t start);
  StringView(std::shared_ptr<std::string> str, size_t start, size_t end);
  size_t size() const;
  char at(size_t pos) const;
  // the str and two substr methods extract the underlying string
  // which results in copying it out of the shared_ptr it is stored in
  std::string str() const;
  std::string substr(size_t start);
  std::string substr(size_t start, size_t count);
  // purely StringView based "substring" which doesn't incur copies
  StringView sub(size_t start);
  StringView sub(size_t start, size_t count);
  // these two operators perform copies and other O(n) operations
  StringView operator+(const StringView& other);
  StringView operator+=(const StringView& other);

  bool operator==(const StringView& other);
  bool operator==(const std::string& other);
  bool operator!=(const StringView& other);
  bool operator!=(const std::string& other);
 private:
  std::shared_ptr<std::string> str_;
  size_t start_;
  size_t end_;
};

} // namespace string
} // namespace ssfs
