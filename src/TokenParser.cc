#include "TokenParser.h"

const CharacterClass tchar{
  '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~',
  // DIGIT
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  // ALPHA
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y','Z'
};

FeedResult TokenParser::feed(std::span<const std::byte> span) {
  if (this->state == TokenParserState::INVALID || this->state == TokenParserState::DONE) {
    return {this->state == TokenParserState::DONE ? FeedState::COMPLETE : FeedState::ERROR, 0};
  }

  std::size_t pos{0};
  while (pos < span.size() && this->state != TokenParserState::DONE) {
    if (!tchar.contains(span[pos]) && this->token.empty()) {
      // Tokens must be at least one TCHAR long
      this->state = TokenParserState::INVALID;
      return {FeedState::ERROR, pos};
    }

    if (!tchar.contains(span[pos])) {
      this->state = TokenParserState::DONE;
      return {FeedState::COMPLETE, pos};
    }

    this->token.push_back(static_cast<char>(span[pos]));
    ++pos;
  }

  if (pos == span.size()) {
    return {FeedState::NEED_MORE_INPUT, pos};
  }

  return {FeedState::COMPLETE, pos};
}

std::optional<std::string> TokenParser::value() const {
  if (state == TokenParserState::DONE) {
    return std::optional<std::string>{this->token};
  }
  return std::nullopt;
}
