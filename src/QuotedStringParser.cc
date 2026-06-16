#include "CharacterClass.h"
#include "QuotedStringParser.h"

const CharacterClass qdtext{{'\t', '\t'}, {' ', ' '}, {0x21, 0x21}, {0x23, 0x5b}, {0x5d, 0x7e}, {0x80, 0xff}};
const CharacterClass quoted_pair_chars{{'\t', '\t'}, {' ', ' '}, {0x21, 0x7e}, {0x80, 0xff}};

std::size_t QuotedStringParser::feed(ByteView bv) {
  if (this->internal_state == QuotedStringParserState::DONE || this->internal_state == QuotedStringParserState::INVALID) {
    return 0;
  }

  std::size_t i{0};
  while (i < bv.size() && this->internal_state != QuotedStringParserState::DONE && this->internal_state != QuotedStringParserState::INVALID) {
    switch (this->internal_state) {
      case QuotedStringParserState::READING_OPENING_QUOTE: {
        if (bv[i] != std::byte{'"'}) {
          this->internal_state = QuotedStringParserState::INVALID;
          return i;
        }

        this->internal_state = QuotedStringParserState::READING_QDTEXT;
        ++i;
        break;
      }

      case QuotedStringParserState::READING_QDTEXT: {
        if (bv[i] == std::byte{'\\'}) {
          ++i;
          this->internal_state = QuotedStringParserState::READING_QUOTED_PAIR_CHAR;
        } else if (bv[i] == std::byte{'"'}) {
          ++i;
          this->internal_state = QuotedStringParserState::DONE;
        } else if (qdtext.contains(bv[i])) {
          this->str.push_back(static_cast<char>(bv[i]));
          ++i;
        } else {
          this->internal_state = QuotedStringParserState::INVALID;
          return i;
        }
        break;
      }

      case QuotedStringParserState::READING_QUOTED_PAIR_CHAR: {
        if (!quoted_pair_chars.contains(bv[i])) {
          this->internal_state = QuotedStringParserState::INVALID;
          return i;
        }

        this->str.push_back(static_cast<char>(bv[i]));
        ++i;

        this->internal_state = QuotedStringParserState::READING_QDTEXT;
        break;
      }
    }
  }

  return i;
}

FeedState QuotedStringParser::state() const {
  switch (this->internal_state) {
    case QuotedStringParserState::DONE: return FeedState::COMPLETE;
    case QuotedStringParserState::INVALID: return FeedState::ERROR;
    default: return FeedState::NEED_MORE_INPUT;
  }
}

std::optional<std::string> QuotedStringParser::value() const {
  if (this->internal_state == QuotedStringParserState::DONE) {
    return std::optional<std::string>{this->str};
  }
  return std::nullopt;
}
