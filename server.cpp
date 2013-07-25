#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "server.h"
#include "http.tab.hpp"

#define SERV_PORT 8080

using namespace sock;
using namespace hcppd;
using namespace std;

extern int yyparse();
extern int yylex_destroy();
extern int yy_scan_string(const char *str);
extern HttpRequest *request;

HttpResponse HttpServer::handleRequest(const HttpRequest& request) {
  HttpResponse response;
  response.status_line.protocol_version = "HTTP/1.1";

  unique_ptr<string> uri = move(request.request_line->uri);
  syslog(LOG_INFO, "Responding to request for: %s", uri->c_str());
  struct stat st;
  if (stat(uri->c_str(), &st) == -1) {
    if (errno == ENOENT) {
      response.status_line.status_code = 404;
      response.status_line.reason_phrase = "Not Found!";
      return response;
    }
    response.status_line.status_code = 500;
    syslog(LOG_ERR, "Error stat-ing directory %s: %m", uri->c_str());
    response.status_line.reason_phrase = "Error stat-ing directory";
    return response;
  }
  stringstream res_ss;
  if (st.st_mode & S_IFDIR) {
    if (uri->at(uri->size()-1) != '/') {
      uri->push_back('/');
    }
    DIR* d;
    if ((d = opendir(uri->c_str())) == NULL) {
      response.status_line.status_code = 500;
      syslog(LOG_ERR, "Error opening directory %s: %m", uri->c_str());
      response.status_line.reason_phrase = "Error opening directory";
      return response;
    }
    struct dirent* dir;
    while ((dir = readdir(d))) {
      string d_name(dir->d_name);
      if (d_name == "." || d_name == "..") {
        continue;
      }
      struct stat st2;
      string d_uri;
      stringstream ss;
      ss << *uri << d_name;
      d_uri = ss.str();
      if (stat(d_uri.c_str(), &st2) == -1) {
        syslog(LOG_ERR, "Could not stat file in directory; %m");
        continue;
      }
      if (st.st_mode & S_IFDIR) {
        res_ss << "<a href=" << d_uri << ">"
               << d_uri << "</a><br>" << endl;
      }
    }
  }
  else if (st.st_mode & S_IFREG) {
    ifstream fin{*uri, ifstream::in};
    string line;
    while (getline(fin, line)) {
      res_ss << line << " <br>" << endl;
    }
  }
  else {
    res_ss << "somethin' else";
  }
  response.status_line.status_code = 200;
  response.status_line.reason_phrase = "OK";
  response.message = res_ss.str();
  return response;
}

HttpRequest HttpServer::parseRequest(const string& requestString) {
  yy_scan_string(requestString.c_str());
  yyparse();
  yylex_destroy();
  return move(*request);
}

HttpResponse HttpServer::handleConnection() {
  std::string msg;
  sock_.Read(&msg);
  return handleRequest(parseRequest(msg));
}

void HttpServer::sendResponse(const HttpResponse& response) {
  sock_.Write(response.format());
}

void HttpServer::serve() {
  sock_.Bind(SERV_PORT);
  sock_.Listen();
  for ( ; ; ) {
    socklen_t clilen;
    sock_.Accept(&clilen);
    int pid = fork();
    if (pid < 0) {
      // error
      syslog(LOG_WARNING, "failed to fork child to handle connection");
    } else if (pid == 0) {
      // child
      close(sock_.getListenFd());
      sendResponse(handleConnection());
      exit(0);
    }
    else {
      // parent
      close(sock_.getConnFd());
    }
  }
}
