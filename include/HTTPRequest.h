#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

#include "FieldCollectionParser.h"
#include "HTTPMessageBody.h"
#include "HTTPMethod.h"
#include "HTTPStatusCode.h"

class HTTPRequest {
  public:
    void append(const std::vector<uint8_t> & chunk);
    bool isValid() const;
    HTTPMethod getMethod() const;
    std::string getRequestTarget() const;
    std::string getHTTPVersion() const;
  private:
    enum class HTTPRequestState {
      REQUEST_LINE,
      HEADERS,
      BODY,
      COMPLETE,
      INVALID
    };

    enum class MessageBodyLengthType {
      UNKNOWN,
      NONE,
      CONTENT_LENGTH,
      CHUNKED,
      INVALID
    };
    MessageBodyLengthType messageBodyLengthType;
    size_t messageBodyLength;
    void setMessageBodyLength();

    HTTPRequestState state = HTTPRequestState::REQUEST_LINE;
    std::optional<HTTPStatusCode> invalidResponseCode;

    std::vector<uint8_t> current_line;

    HTTPMethod method;
    std::string requestTarget;
    std::string httpVersion;

    FieldCollectionParser field_collection_parser;
    FieldCollection headers;
    HTTPMessageBody body;
};

#endif
