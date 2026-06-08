#ifndef TOKEN_PARSER_H
#define TOKEN_PARSER_H

#include <optional>
#include <span>
#include <string>

#include "CharacterClass.h"
#include "FeedResult.h"

extern const CharacterClass tchar;

class TokenParser {
  public:
    FeedResult feed(std::span<const std::byte> span);
    std::optional<std::string> value() const;

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
