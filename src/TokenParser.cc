#include "TokenParser.h"

const CharacterClass tchar{
  '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~',
  // DIGIT
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  // ALPHA
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y','Z'
};

std::size_t TokenParser::feed(ByteView bv) {
  if (this->internal_state == TokenParserState::INVALID || this->internal_state == TokenParserState::DONE) {
    return 0;
  }

  std::size_t pos{0};
  while (pos < bv.size() && this->internal_state != TokenParserState::DONE) {
    if (!tchar.contains(bv[pos]) && this->token.empty()) {
      // Tokens must be at least one TCHAR long
      this->internal_state = TokenParserState::INVALID;
      return pos;
    }

    if (!tchar.contains(bv[pos])) {
      this->internal_state = TokenParserState::DONE;
      return pos;
    }

    this->token.push_back(static_cast<char>(bv[pos]));
    ++pos;
  }

  return pos;
}

FeedState TokenParser::state() const {
  switch (this->internal_state) {
    case TokenParserState::DONE: return FeedState::COMPLETE;
    case TokenParserState::INVALID: return FeedState::ERROR;
    default: return FeedState::NEED_MORE_INPUT;
  }
}

std::optional<std::string> TokenParser::value() const {
  if (this->internal_state == TokenParserState::DONE) {
    return std::optional<std::string>{this->token};
  }
  return std::nullopt;
}
