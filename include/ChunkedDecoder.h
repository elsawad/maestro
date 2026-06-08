#ifndef CHUNKED_DECODER_H
#define CHUNKED_DECODER_H

#include <cstdint>
#include <list>
#include <variant>
#include <vector>

#include "Chunk.h"
#include "FieldCollection.h"
#include "FieldCollectionParser.h"
#include "QuotedStringParser.h"
#include "StreamProcessor.h"
#include "TokenParser.h"

class ChunkedDecoder: public StreamProcessor {
  public:
    FeedResult feed(std::span<const std::byte> span) override;

    struct Handler {
      virtual ~Handler() = default;
      virtual void on_chunk(Chunk && chunk);
      virtual void on_data(std::span<const std::byte> data);
      virtual void on_error();
      virtual void on_finish();
      virtual void on_trailer_section(FieldCollection && trailer_section);
    };

    void set_handler(Handler & handler);
    void unset_handler();

  private:
    enum class State {
      READING_CHUNK_SIZE,
      READING_CHUNK_EXT,
      READING_CHUNK_EXT_SEMICOLON_BWS,
      READING_CHUNK_EXT_NAME_BWS,
      READING_CHUNK_EXT_NAME,
      READING_CHUNK_EXT_REST_BWS,
      READING_CHUNK_EXT_VAL_BWS,
      READING_CHUNK_EXT_VAL,
      READING_CHUNK_SIZE_LF,
      READING_CHUNK_DATA,
      READING_CHUNK_DATA_CR,
      READING_CHUNK_DATA_LF,
      READING_TRAILER_SECTION,
      READING_CHUNKED_BODY_CR,
      READING_CHUNKED_BODY_LF,
      DONE,
      INVALID
    };
    State state = State::READING_CHUNK_SIZE;

    static Handler default_handler;
    Handler * handler{&default_handler};

    Chunk partial_chunk;
    bool is_last_chunk{false};

    std::vector<std::byte> buffer;
    std::size_t remaining_chunk_size = 0;

    std::optional<TokenParser> chunk_ext_name_parser;
    std::optional<std::variant<TokenParser, QuotedStringParser>> chunk_ext_val_parser;

    std::optional<FieldCollectionParser> field_collection_parser;
};

#endif
