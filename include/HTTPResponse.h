#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <ostream>
#include <string>
#include <unordered_map>

enum class HTTPStatusCode;
class HTTPRequest;

class HTTPResponse {
  public:
    HTTPResponse(HTTPRequest & req);
    HTTPResponse & body();
    HTTPResponse & header(std::string const &key, std::string const &value);
    HTTPResponse & status(HTTPStatusCode status);
    std::string & str();
  private:
    HTTPStatusCode status_p;
    std::unordered_map<std::string, std::string> headers_p;
    std::ostream body_p;
};

#endif
