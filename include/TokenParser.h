#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <string>
#include <string_view>

#include "CharacterClass.h"
#include "FeedResult.h"

extern const CharacterClass tchar;

class TokenParser {
  public:
    FeedResult feed(std::string_view input);
    std::optional<std::string_view> value() const;

  private:
    enum class TokenParserState {
      READING_TOKEN,
      DONE,
      INVALID
    };

    TokenParserState state{TokenParserState::READING_TOKEN};
    std::string token;
};

#endif
