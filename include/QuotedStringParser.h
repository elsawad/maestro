#ifndef QUOTED_STRING_PARSER_H
#define QUOTED_STRING_PARSER_H

#include <optional>
#include <span>
#include <string>

#include "FeedResult.h"

class QuotedStringParser {
  public:
    FeedResult feed(std::span<const std::byte> span);
    std::optional<std::string> value() const;

  private:
    enum class QuotedStringParserState {
      READING_OPENING_QUOTE,
      READING_QDTEXT,
      READING_QUOTED_PAIR_CHAR,
      DONE,
      INVALID
    };
    QuotedStringParserState state{QuotedStringParserState::READING_OPENING_QUOTE};
    std::string str;
};

#endif
