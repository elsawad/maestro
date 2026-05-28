#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include <memory>

#include "HTTPMessageBody.h"
#include "HTTPMessageHeaders.h"

class HTTPMessage {
  private:
    std::unique_ptr<HTTPMessageHeaders> headers;
    std::unique_ptr<HTTPMessageBody> body;
};

#endif
