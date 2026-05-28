#ifndef HTTPMESSAGEBODY_H
#define HTTPMESSAGEBODY_H

#include <cstdint>
#include <vector>

class HTTPMessageBody {
  public:
    void append(const std::vector<uint8_t>::const_iterator & chunkBegin, const std::vector<uint8_t>::const_iterator & chunkEnd);
    size_t size() const;
    std::vector<uint8_t> vector() const;

    enum class HTTPMessageBodyType {
      NONE,
      CONTENT_LENGTH,
      CHUNKED
    };

    HTTPMessageBodyType getType() const;
    void setType(HTTPMessageBodyType type);
  private:
    std::vector<uint8_t> body_vector;
    HTTPMessageBodyType type;
};

#endif
