#pragma once

#include <string>

#include "http_request.h"
#include "lib/socket.h"

namespace hcppd {

class HttpServer {
public:
  std::string port = "80";
  std::string dynamic_content_server;
  void serve();
private:
  std::string handleRequest(const HttpRequest& request);
  HttpRequest parseRequest(const std::string& requestString);
  std::string handleConnection();
  std::unique_ptr<lib::sock::Socket> sock_;
};
} // server
