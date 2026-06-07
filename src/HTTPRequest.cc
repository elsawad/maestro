#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ABNF.h"
#include "HTTPRequest.h"
#include "TokenParser.h"

const std::unordered_map<std::string, HTTPMethod> method_map{{
  {"GET", HTTPMethod::GET},
  {"POST", HTTPMethod::POST},
  {"PUT", HTTPMethod::PUT},
  {"DELETE", HTTPMethod::DELETE},
  {"HEAD", HTTPMethod::HEAD},
  {"OPTIONS", HTTPMethod::OPTIONS},
  {"TRACE", HTTPMethod::TRACE},
  {"CONNECT", HTTPMethod::CONNECT}
}};

// https://stackoverflow.com/a/66846049
bool nextToken(const std::string &s, std::string::size_type &start, std::string &token) {
  token.clear();

  start = s.find_first_not_of(" \t", start);
  if (start == std::string::npos) {
    return false;
  }

  std::string::size_type end;

  if (s[start] == '\'') {
    ++start;
    end = s.find('\'', start);
  } else {
    end = s.find_first_of(" \t,", start);
  }

  if (end == std::string::npos) {
    token = s.substr(start);
    start = s.size();
  } else {
    token = s.substr(start, end-start);
    if ((s[end] != ',') && ((end = s.find(',', end + 1)) == std::string::npos)) {
      start = s.size();
    } else {
      start = end + 1;
    }
  }
  return true;
}

std::vector<std::string> getFieldValues(const std::vector<std::pair<std::string, std::string>> & headers, const std::string & field_name) {
  std::vector<std::string> fieldValues;
  std::string::size_type start{0};
  std::string token;
  for (auto header{headers.cbegin()}; header != headers.cend(); header = std::find_if(std::next(header), headers.cend(), [&field_name](const auto & header) { return header.first == field_name; })) {
    start = 0;
    token.clear();

    while (nextToken(header->second, start, token)) {
      fieldValues.push_back(token);
    }
  }
  return fieldValues;
}

void HTTPRequest::setMessageBodyLength() {
  // Implementation for determining the message body length based on the headers
  // according to RFC 9112 6.3
  std::vector<std::string_view> transferEncodingFieldValues{this->headers.get_all("Transfer-Encoding")};
  std::vector<std::string_view> contentLengthFieldValues{this->headers.get_all("Content-Length")};

  // Only conditions that apply to requests are considered here

  // If both Transfer-Encoding and Content-Length headers are present,
  // Content-Length is ignored and the message body length is determined by Transfer-Encoding

  // If a Transfer-Encoding header field is present and the chunked transfer coding is the final encoding, the message body length is determined by reading and decoding the chunked data until the transfer coding indicates the data is complete.
  if (!transferEncodingFieldValues.empty() && transferEncodingFieldValues.back() == "chunked") {
    this->messageBodyLengthType = MessageBodyLengthType::CHUNKED;
    return;
  }

  // If a Transfer-Encoding header field is present in a request and the chunked transfer coding is not the final encoding, the message body length cannot be determined reliably; the server MUST respond with the 400 (Bad Request) status code and then close the connection.
  if (!transferEncodingFieldValues.empty() && transferEncodingFieldValues.back() != "chunked") {
    this->messageBodyLengthType = MessageBodyLengthType::INVALID;
    this->invalidResponseCode = HTTPStatusCode::BAD_REQUEST;
    return;
  }

  // At this point, no Transfer-Encoding header field is present.

  // If a message is received without Transfer-Encoding and with an invalid Content-Length header field, then the message framing is invalid and the recipient MUST treat it as an unrecoverable error, unless the field value can be successfully parsed as a comma-separated list (Section 5.6.1 of [HTTP]), all values in the list are valid, and all values in the list are the same (in which case, the message is processed with that single value used as the Content-Length field value). If the unrecoverable error is in a request message, the server MUST respond with a 400 (Bad Request) status code and then close the connection. If it is in a response message received by a proxy, the proxy MUST close the connection to the server, discard the received response, and send a 502 (Bad Gateway) response to the client. If it is in a response message received by a user agent, the user agent MUST close the connection to the server and discard the received response.
  // If a valid Content-Length header field is present without Transfer-Encoding, its decimal value defines the expected message body length in octets. If the sender closes the connection or the recipient times out before the indicated number of octets are received, the recipient MUST consider the message to be incomplete and close the connection.
  if (!contentLengthFieldValues.empty()) {
    std::optional<size_t> contentLength{std::nullopt};
    for (const std::string_view & contentLengthFieldValue : contentLengthFieldValues) {
      if (!std::all_of(contentLengthFieldValue.cbegin(), contentLengthFieldValue.cend(), [](char c) { return DIGIT.contains(c); })) {
        this->messageBodyLengthType = MessageBodyLengthType::INVALID;
        this->invalidResponseCode = HTTPStatusCode::BAD_REQUEST;
        return;
      }

      size_t temp{std::stoul(std::string{contentLengthFieldValue})};
      if (contentLength.has_value() && temp != contentLength.value()) {
        this->messageBodyLengthType = MessageBodyLengthType::INVALID;
        this->invalidResponseCode = HTTPStatusCode::BAD_REQUEST;
        return;
      }
      contentLength = temp;
    }

    this->messageBodyLengthType = MessageBodyLengthType::CONTENT_LENGTH;
    this->messageBodyLength = contentLength.value();
    return;
  }

  // If this is a request message and none of the above are true, then the message body length is zero (no message body is present).
  this->messageBodyLengthType = MessageBodyLengthType::NONE;
  return;
}

void HTTPRequest::append(const std::vector<uint8_t> & chunk) {
  if (this->state == HTTPRequestState::COMPLETE || this->state == HTTPRequestState::INVALID) {
    // No more data should be appended
    return;
  }

  if (chunk.empty()) {
    // No data to append
    return;
  }

  // input should contain the current line (if any) followed by the new chunk of data
  // Once we reach the body section, however, we should not consider the concept of lines anymore and just append the data to the body until we have read the expected number of bytes or have decoded the chunked data completely
  std::vector<uint8_t> input(std::move(this->current_line));
  input.insert(input.cend(), chunk.cbegin(), chunk.cend());

  auto line_start = input.cbegin();

  const std::string crlf_str("\r\n");
  auto crlf(std::search(line_start, input.cend(), crlf_str.cbegin(), crlf_str.cend()));
  if (crlf == input.cend() && this->state != HTTPRequestState::BODY) {
    // No CRLF found, save current line and wait for more data
    this->current_line = std::move(input);
    return;
  }

  switch (this->state) {
    case HTTPRequestState::REQUEST_LINE: {
      // Implementation for parsing the request line
      const auto sp1(std::find(line_start, crlf, ' '));
      if (sp1 == crlf) {
        // No space found, invalid request line
        this->state = HTTPRequestState::INVALID;
        return;
      }

      const std::string method_str(line_start, sp1);
      if (method_map.find(method_str) == method_map.cend()) {
        // No space found, invalid HTTP method
        this->state = HTTPRequestState::INVALID;
        return;
      }
      this->method = method_map.at(method_str);

      const auto sp2(std::find(sp1 + 1, crlf, ' '));
      if (sp2 == crlf) {
        // No second space found, invalid request line
        this->state = HTTPRequestState::INVALID;
        return;
      }
      this->requestTarget = std::move(std::string(sp1 + 1, sp2));

      const std::string httpPrefixStr("HTTP/");
      const auto httpPrefix(std::search(sp2 + 1, crlf, httpPrefixStr.cbegin(), httpPrefixStr.cend()));
      if (httpPrefix != sp2 + 1) {
        // HTTP version does not start with "HTTP/", invalid request line
        this->state = HTTPRequestState::INVALID;
        return;
      }

      const std::string httpVersion(httpPrefix, crlf);
      if (httpVersion.length() != 8 || httpVersion.compare(0, 5, "HTTP/") || !std::isdigit(httpVersion[5]) || httpVersion[6] != '.' || !std::isdigit(httpVersion[7])) {
        // HTTP version is not in the format "HTTP/X.Y", invalid request line
        this->state = HTTPRequestState::INVALID;
        return;
      }
      this->httpVersion = std::move(httpVersion);

      // Setup for parsing headers
      line_start = crlf + 2;
      crlf = std::search(line_start, input.cend(), crlf_str.cbegin(), crlf_str.cend());
      if (crlf == input.cend()) {
        // No CRLF found, save current line and wait for more data
        input.erase(input.cbegin(), line_start);
        this->current_line = std::move(input);
        return;
      }

      this->state = HTTPRequestState::HEADERS;
      [[fallthrough]];
    }

    case HTTPRequestState::HEADERS: {
      // Keep reading headers until
      // (1) there is no CRLF found, indicating an incomplete header; or
      // (2) the CRLF is at the start of the line, indicating the end of the headers section of the request
      while (crlf != input.cend() && crlf != line_start) {
        // A header line is in the format "field-name: OWS field-value OWS", where OWS is optional whitespace (spaces or tabs)
        // A field-name is a token, which is just a string of 1 or more TCHARs
        const auto colon(std::find_if(line_start, crlf, [](char c) { return tchar.contains(c); }));
        if (colon == crlf or *colon != ':') {
          // No colon found, invalid header line
          this->state = HTTPRequestState::INVALID;
          this->invalidResponseCode = HTTPStatusCode::BAD_REQUEST;
          return;
        }

        const std::string header_name(line_start, colon);

        const auto header_value_start{std::find_if(colon + 1, crlf, [](char c) { return WSP.contains(c); })};
        const auto header_value_end{std::find_if(std::make_reverse_iterator(crlf), std::make_reverse_iterator(header_value_start), [](char c) { return WSP.contains(c); }).base()};
        const std::string header_value(header_value_start, header_value_end);

        this->headers.emplace_back(std::move(header_name), std::move(header_value));

        line_start = crlf + 2;
        crlf = std::search(line_start, input.cend(), crlf_str.cbegin(), crlf_str.cend());
      }

      if (crlf == input.cend()) {
        // No CRLF found, save current line and wait for more data
        input.erase(input.cbegin(), line_start);
        this->current_line = std::move(input);
        return;
      }

      // CRLF is at the start of the line, indicating the end of the headers section
      // Setup for parsing body
      line_start = crlf + 2;

      this->setMessageBodyLength();
      if (this->messageBodyLengthType == MessageBodyLengthType::INVALID) {
        this->state = HTTPRequestState::INVALID;
        return;
      }

      this->state = HTTPRequestState::BODY;
      [[fallthrough]];
    }

    case HTTPRequestState::BODY: {
      // Implementation for parsing the body
      switch (this->messageBodyLengthType) {
        case MessageBodyLengthType::NONE: {
          // No body is present, complete the request
          break;
        }

        case MessageBodyLengthType::CONTENT_LENGTH: {
          // current_line should be empty because we save all bytes in the body as they come until we reach Content-Length
          this->current_line.clear();

          if (this->body.size() + std::distance(line_start, input.cend()) <= this->messageBodyLength) {
            this->body.append(line_start, input.cend());
            input.clear();
            this->current_line = std::move(input);
            return;
          }

          const size_t remainingBytes{this->messageBodyLength - this->body.size()};
          this->body.append(line_start, line_start + remainingBytes);
          break;
        }

        case MessageBodyLengthType::CHUNKED: {

          break;
        }

        default: {
          // Invalid message body length type, should not happen
          this->state = HTTPRequestState::INVALID;
          return;
        }
      }

      this->state = HTTPRequestState::COMPLETE;
    }
  }
}

bool HTTPRequest::isValid() const {
  // Implementation for validating the HTTP request
  return this->state == HTTPRequestState::COMPLETE;
}

HTTPMethod HTTPRequest::getMethod() const {
  // Implementation for returning the HTTP method as a string
  return this->method;
}

std::string HTTPRequest::getRequestTarget() const {
  // Implementation for returning the request target
  return this->requestTarget;
}

std::string HTTPRequest::getHTTPVersion() const {
  // Implementation for returning the HTTP version
  return this->httpVersion;
}
