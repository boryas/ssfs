#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lib/http/request.h"

namespace lib {
namespace http {
namespace parse {

class ParserError : public std::runtime_error {
 public:
  ParserError(const std::string& reason)
    : std::runtime_error("http request ParserError: " + reason) {}
};

enum class ParserState {
  REQUEST_LINE_METHOD,
  REQUEST_LINE_URI,
  REQUEST_LINE_PROT,
  HEADER,
  BODY,
  DONE,
};

class Parser {
 public:
  bool hungry() const;
  void consume(std::unique_ptr<std::string> chunk);
  HttpRequest request() const;
 private:
  void consumeToken(const std::string& token);
  ParserState state_ = ParserState::REQUEST_LINE_METHOD;
  HttpRequest req_;
  std::vector<std::shared_ptr<std::string>> chunks_;
};

} // namespace parse
} // namespace http
} // namespace lib