#include "blog.h"

#include <sys/types.h>
#include <unistd.h>
#include <sstream>

#include "lib/daemon.h"
#include "lib/fs.h"
#include "lib/http/parse/parser.h"
#include "lib/options.h"
#include "resource.h"

namespace servers {
namespace blog {

auto constexpr TEMPLATE_FILE = "/home/bb/blog/templates/post.html";

BlogServer::BlogServer(const std::string& port)
 : lib::server::BlockingServer<BlogServer>(port) {
   auto t = std::make_unique<std::string>(
       lib::fs::readFile(TEMPLATE_FILE));
   template_.reset(new lib::html::HtmlTemplate(std::move(t)));
}

lib::http::HttpResponse BlogServer::handleRequest(
    const lib::http::HttpRequest& request) const {
  syslog(LOG_INFO, "Responding to request for: %s", request.uri().c_str());
  try {
    auto resource_type = resolveResourceType(request.uri());
    if (resource_type == ResourceType::BLOG_POST) {
      BlogPost post(request.uri());
      std::unordered_map<std::string, std::string> contents = {
        {"title", request.uri()},
        {"post", post.get()}
      };
      auto html = template_->populate(contents);
      return lib::http::HttpResponse(html, "text/html");
    } else {
      Css css(request.uri());
      return lib::http::HttpResponse(css.get(), "text/css");
    }
  } catch (lib::fs::PathNotFoundError& e) {
    lib::http::HttpResponse response(404, "Not Found!", "404 LOL");
    return response;
  } catch (std::exception& e) {
    lib::http::HttpResponse response(500, "Server Error!", "500 LOL");
    return response;
  }
}

std::string BlogServer::handle(std::unique_ptr<lib::sock::Socket> conn) {
  auto buf = std::make_unique<std::string>();
  buf->resize(4096);
  conn->readn(*buf, 4096);
  lib::http::parse::Parser http_parser;
  http_parser.consume(std::move(buf));
  auto r = handleRequest(http_parser.request()).format();
  syslog(LOG_INFO, "Responding with: \n%s", r.c_str());
  conn->write_(r);
  return r;
}

} //namespace blog
} //namespace servers


int main(int argc, char **argv) {
  lib::daemon::daemonize();
  auto options = lib::options::get_options(argc, argv);
  lib::options::log_options(options);
  servers::blog::BlogServer server(options["port"]);
  server.serve();
}
