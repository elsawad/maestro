#ifndef FIELD_COLLECTION_PARSER_H
#define FIELD_COLLECTION_PARSER_H

#include "FieldCollection.h"
#include "IncrementalConsumer.h"
#include "TokenParser.h"

class FieldCollectionParser {
  public:
    std::size_t feed(ByteView);
    FeedState state() const;
    FieldCollection & value();

  private:
    enum class FieldCollectionParserState {
      READING_FIELD_NAME,
      READING_OWS_BEFORE_FIELD_VALUE,
      READING_FIELD_VALUE,
      READING_FIELD_LINE_LF,
      DONE,
      INVALID
    };

    FieldCollectionParserState internal_state{FieldCollectionParserState::READING_FIELD_NAME};

    std::optional<TokenParser> field_name_parser;

    std::string field_name;
    std::string field_value;

    FieldCollection field_collection;
};

static_assert(IncrementalConsumer<FieldCollectionParser>);

#endif
