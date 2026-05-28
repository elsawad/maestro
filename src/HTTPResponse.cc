#include "HTTPResponse.h"

HTTPResponse & HTTPResponse::status(HTTPStatusCode status) {
  this->status_p = status;
  return *this;
}

HTTPResponse & HTTPResponse::header(std::string const &key, std::string const &value) {
  // Implementation for setting headers
  this->headers_p[key] = value;
  return *this;
}

HTTPResponse & HTTPResponse::body() {
  // Implementation for setting body content
  return *this;
}
