#include "HTTPMessageBody.h"

void HTTPMessageBody::append(const std::vector<uint8_t>::const_iterator & chunkBegin, const std::vector<uint8_t>::const_iterator & chunkEnd) {
  this->body_vector.insert(this->body_vector.cend(), chunkBegin, chunkEnd);
}

size_t HTTPMessageBody::size() const {
  return this->body_vector.size();
}

std::vector<uint8_t> HTTPMessageBody::vector() const {
  return this->body_vector;
}
