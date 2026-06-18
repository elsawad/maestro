#ifndef QUOTED_STRING_PARSER_H
#define QUOTED_STRING_PARSER_H

#include <optional>
#include <string>

#include "IncrementalConsumer.h"

class QuotedStringParser {
  public:
    std::size_t feed(ByteView bv);
    FeedState state() const;
    std::optional<std::string> value() const;

  private:
    enum class QuotedStringParserState {
      READING_OPENING_QUOTE,
      READING_QDTEXT,
      READING_QUOTED_PAIR_CHAR,
      DONE,
      INVALID
    };
    QuotedStringParserState internal_state{QuotedStringParserState::READING_OPENING_QUOTE};
    std::string str;
};

static_assert(IncrementalConsumer<QuotedStringParser>);

#endif
