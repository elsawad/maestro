#ifndef QUOTED_STRING_PARSER_H
#define QUOTED_STRING_PARSER_H

#include <optional>
#include <string>
#include <string_view>

#include "FeedResult.h"

class QuotedStringParser {
  public:
    FeedResult feed(std::string_view sv);
    std::optional<std::string_view> get_string() const;

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
