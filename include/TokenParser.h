#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <string>

#include "CharacterClass.h"
#include "IncrementalConsumer.h"

extern const CharacterClass tchar;

class TokenParser {
  public:
    std::size_t feed(ByteView bv);
    FeedState state() const;
    std::optional<std::string> value() const;

  private:
    enum class TokenParserState {
      READING_TOKEN,
      DONE,
      INVALID
    };

    TokenParserState internal_state{TokenParserState::READING_TOKEN};
    std::string token;
};

static_assert(IncrementalConsumer<TokenParser>);

#endif
