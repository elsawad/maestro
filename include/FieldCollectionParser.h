#ifndef FIELD_COLLECTION_PARSER_H
#define FIELD_COLLECTION_PARSER_H

#include <cstdint>
#include <span>
#include <vector>

#include "FieldCollection.h"
#include "StreamProcessor.h"
#include "TokenParser.h"

class FieldCollectionParser: public StreamProcessor {
  public:
    FeedResult feed(std::span<const std::byte> span) override;
    FieldCollection & value();
  private:
    enum class State {
      READING_FIELD_NAME,
      READING_OWS_BEFORE_FIELD_VALUE,
      READING_FIELD_VALUE,
      READING_FIELD_LINE_LF,
      DONE,
      INVALID
    };

    State state = State::READING_FIELD_NAME;

    std::optional<TokenParser> field_name_parser;

    std::string field_name;
    std::string field_value;

    FieldCollection field_collection;
};

#endif
