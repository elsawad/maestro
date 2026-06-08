#include "CharacterClass.h"
#include "QuotedStringParser.h"

const CharacterClass qdtext{{'\t', '\t'}, {' ', ' '}, {0x21, 0x21}, {0x23, 0x5b}, {0x5d, 0x7e}, {0x80, 0xff}};
const CharacterClass quoted_pair_chars{{'\t', '\t'}, {' ', ' '}, {0x21, 0x7e}, {0x80, 0xff}};

FeedResult QuotedStringParser::feed(std::span<const std::byte> span) {
  if (this->state == QuotedStringParserState::DONE || this->state == QuotedStringParserState::INVALID) {
    return {this->state == QuotedStringParserState::DONE ? FeedState::COMPLETE : FeedState::ERROR, 0};
  }

  std::size_t i{0};
  while (i < span.size() && this->state != QuotedStringParserState::DONE) {
    switch (this->state) {
      case QuotedStringParserState::READING_OPENING_QUOTE: {
        if (span[i] != std::byte{'"'}) {
          this->state = QuotedStringParserState::INVALID;
          return {FeedState::ERROR, i};
        }

        this->state =QuotedStringParserState::READING_QDTEXT;
        ++i;
        break;
      }

      case QuotedStringParserState::READING_QDTEXT: {
        if (span[i] == std::byte{'\\'}) {
          ++i;
          this->state = QuotedStringParserState::READING_QUOTED_PAIR_CHAR;
        } else if (span[i] == std::byte{'"'}) {
          ++i;
          this->state = QuotedStringParserState::DONE;
        } else if (qdtext.contains(span[i])) {
          this->str.push_back(static_cast<char>(span[i]));
          ++i;
        } else {
          this->state = QuotedStringParserState::INVALID;
          return {FeedState::ERROR, i};
        }
        break;
      }

      case QuotedStringParserState::READING_QUOTED_PAIR_CHAR: {
        if (!quoted_pair_chars.contains(span[i])) {
          this->state = QuotedStringParserState::INVALID;
          return {FeedState::ERROR, i};
        }

        this->str.push_back(static_cast<char>(span[i]));
        ++i;

        this->state = QuotedStringParserState::READING_QDTEXT;
        break;
      }
    }
  }

  if (this->state == QuotedStringParserState::DONE) {
    return {FeedState::COMPLETE, i};
  }

  return {FeedState::NEED_MORE_INPUT, i};
}

std::optional<std::string> QuotedStringParser::value() const {
  if (this->state == QuotedStringParserState::DONE) {
    return std::optional<std::string>{this->str};
  }
  return std::nullopt;
}
