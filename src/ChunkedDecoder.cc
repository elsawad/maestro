#include <algorithm>
#include <string>
#include <vector>

#include "ABNF.h"
#include "ChunkedDecoder.h"
#include "FieldCollectionParser.h"

// Feed a chunk of data to the decoder.
std::size_t ChunkedDecoder::feed(ByteView bv) {
  std::size_t pos{0};
  while (pos < bv.size() && this->internal_state != ChunkedDecoderState::DONE && this->internal_state != ChunkedDecoderState::INVALID) {
    switch(this->internal_state) {
      case ChunkedDecoderState::READING_CHUNK_SIZE: {
        while (pos < bv.size() && HEXDIG.contains(bv[pos])) {
          this->buffer.push_back(bv[pos]);
          ++pos;
        }

        if (pos == bv.size()) {
          // No non-HEXDIG character found, wait for more data
          return pos;
        }

        // We found a non-HEXDIG character, so everything before this character is the chunk size in hexadecimal
        if (this->buffer.empty()) {
          // No chunk size found, invalid chunked encoding
          this->internal_state = ChunkedDecoderState::INVALID;
          this->handler->on_error();
          break;
        }

        const std::string chunk_size_str{reinterpret_cast<const char *>(this->buffer.data()), this->buffer.size()};
        this->buffer.clear();
        this->remaining_chunk_size = std::stoull(chunk_size_str, nullptr, 16);

        this->is_last_chunk = this->remaining_chunk_size == 0;
        this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT;
        break;
      }

      case ChunkedDecoderState::READING_CHUNK_EXT: {
        if (bv[pos] == std::byte{'\r'}) {
          // No chunk extension, we expect to see a CRLF immediately after the chunk size
          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_SIZE_LF;
          break;
        }

        if (WSP.contains(bv[pos])) {
          // There is a chunk extension and it begins with BWS
          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_SEMICOLON_BWS;
          break;
        }

        if (bv[pos] == std::byte{';'}) {
          // There is a chunk extension and it begins with a semicolon
          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_NAME_BWS;
          break;
        }

        this->internal_state = ChunkedDecoderState::INVALID;
        this->handler->on_error();
        break;
      }

      case ChunkedDecoderState::READING_CHUNK_EXT_SEMICOLON_BWS:
      case ChunkedDecoderState::READING_CHUNK_EXT_NAME_BWS:
      case ChunkedDecoderState::READING_CHUNK_EXT_REST_BWS:
      case ChunkedDecoderState::READING_CHUNK_EXT_VAL_BWS: {
        while (pos < bv.size() && WSP.contains(bv[pos])) {
          ++pos;
        }

        if (pos == bv.size()) {
          // We only found whitespace, wait for more data
          return pos;
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_EXT_SEMICOLON_BWS && bv[pos] == std::byte{';'}) {
          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_NAME_BWS;
          break;
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_EXT_NAME_BWS && tchar.contains(bv[pos])) {
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_NAME;
          break;
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_EXT_REST_BWS && bv[pos] == std::byte{';'}) {
          // No extension value and extensions continue
          const std::string chunk_ext_name{*this->chunk_ext_name_parser->value()};
          this->chunk_ext_name_parser.reset();

          this->partial_chunk.extensions.emplace_back(chunk_ext_name, std::nullopt);

          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_NAME_BWS;
          break;
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_EXT_REST_BWS && bv[pos] == std::byte{'='}) {
          // There is an extension value
          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_VAL_BWS;
          break;
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_EXT_REST_BWS && bv[pos] == std::byte{'\r'}) {
          // No chunk extension value and extensions end, we expect to see a CRLF immediately after the chunk extension name
          const std::string chunk_ext_name{*this->chunk_ext_name_parser->value()};
          this->chunk_ext_name_parser.reset();

          this->partial_chunk.extensions.emplace_back(chunk_ext_name, std::nullopt);

          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNK_SIZE_LF;
          break;
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_EXT_VAL_BWS && (tchar.contains(bv[pos]) || bv[pos] == std::byte{'"'})) {
          this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_VAL;
          break;
        }

        this->internal_state = ChunkedDecoderState::INVALID;
        this->handler->on_error();
        break;
      }

      case ChunkedDecoderState::READING_CHUNK_EXT_NAME: {
        if (!this->chunk_ext_name_parser.has_value()) {
          this->chunk_ext_name_parser.emplace();
        }

        const std::size_t consumed{this->chunk_ext_name_parser->feed(bv.subspan(pos))};

        switch (this->chunk_ext_name_parser->state()) {
          case FeedState::NEED_MORE_INPUT: {
            return bv.size();
          }

          case FeedState::COMPLETE: {
            pos += consumed;
            this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT_REST_BWS;
            break;
          }

          case FeedState::ERROR: {
            this->internal_state = ChunkedDecoderState::INVALID;
            this->handler->on_error();
            break;
          }
        }

        break;
      }

      case ChunkedDecoderState::READING_CHUNK_EXT_VAL: {
        if (!this->chunk_ext_val_parser.has_value() && tchar.contains(bv[pos])) {
          this->chunk_ext_val_parser.emplace(std::in_place_type<TokenParser>);
        } else if (!this->chunk_ext_val_parser.has_value() && bv[pos] == std::byte{'"'}) {
          this->chunk_ext_val_parser.emplace(std::in_place_type<QuotedStringParser>);
        } else if (!this->chunk_ext_val_parser.has_value()) {
          this->internal_state = ChunkedDecoderState::INVALID;
          this->handler->on_error();
          break;
        }

        const std::size_t consumed{std::visit([bv, pos](auto & parser) {
          return parser.feed(bv.subspan(pos));
        }, *this->chunk_ext_val_parser)};

        const FeedState chunk_ext_val_parser_state{std::visit([bv, pos](auto & parser) {
          return parser.state();
        }, *this->chunk_ext_val_parser)};

        if (chunk_ext_val_parser_state == FeedState::NEED_MORE_INPUT) {
          return bv.size();
        }

        if (chunk_ext_val_parser_state == FeedState::ERROR) {
          this->internal_state = ChunkedDecoderState::INVALID;
          this->handler->on_error();
          break;
        }

        const std::string chunk_ext_name{*this->chunk_ext_name_parser->value()};
        this->chunk_ext_name_parser.reset();

        const std::string chunk_ext_val{std::visit(
          [](const auto & parser) -> std::string {
            return *parser.value();
          },
          *this->chunk_ext_val_parser
        )};
        this->chunk_ext_val_parser.reset();

        this->partial_chunk.extensions.emplace_back(chunk_ext_name, chunk_ext_val);

        pos += consumed;
        this->internal_state = ChunkedDecoderState::READING_CHUNK_EXT;
        break;
      }

      case ChunkedDecoderState::READING_CHUNK_DATA_CR:
      case ChunkedDecoderState::READING_CHUNKED_BODY_CR: {
        if (bv[pos] != std::byte{'\r'}) {
          this->internal_state = ChunkedDecoderState::INVALID;
          this->handler->on_error();
          break;
        }

        ++pos;
        this->internal_state =
            this->internal_state == ChunkedDecoderState::READING_CHUNK_DATA_CR ? ChunkedDecoderState::READING_CHUNK_DATA_LF
          : ChunkedDecoderState::READING_CHUNKED_BODY_LF;
        break;
      }

      case ChunkedDecoderState::READING_CHUNK_SIZE_LF:
      case ChunkedDecoderState::READING_CHUNK_DATA_LF:
      case ChunkedDecoderState::READING_CHUNKED_BODY_LF: {
        if (bv[pos] != std::byte{'\n'}) {
          this->internal_state = ChunkedDecoderState::INVALID;
          this->handler->on_error();
          break;
        }

        if ((this->internal_state == ChunkedDecoderState::READING_CHUNK_DATA_LF && !this->is_last_chunk) || (this->internal_state == ChunkedDecoderState::READING_CHUNK_SIZE_LF && this->is_last_chunk)) {
          this->handler->on_chunk(std::move(this->partial_chunk));
        }

        if (this->internal_state == ChunkedDecoderState::READING_CHUNK_DATA_LF && !this->is_last_chunk) {
          this->partial_chunk = Chunk{};
        }

        ++pos;
        this->internal_state =
            this->internal_state == ChunkedDecoderState::READING_CHUNK_SIZE_LF && this->is_last_chunk ? ChunkedDecoderState::READING_TRAILER_SECTION
          : this->internal_state == ChunkedDecoderState::READING_CHUNK_SIZE_LF && !this->is_last_chunk ? ChunkedDecoderState::READING_CHUNK_DATA
          : this->internal_state == ChunkedDecoderState::READING_CHUNK_DATA_LF ? ChunkedDecoderState::READING_CHUNK_SIZE
          : ChunkedDecoderState::DONE;

        if (this->internal_state == ChunkedDecoderState::DONE) {
          this->handler->on_finish();
        }

        break;
      }

      case ChunkedDecoderState::READING_CHUNK_DATA: {
        const std::size_t bytes_available = bv.size() - pos;

        if (bytes_available == 0) {
          return bv.size();
        }

        if (bytes_available < this->remaining_chunk_size) {
          const auto new_data = bv.subspan(pos);
          this->handler->on_data(new_data);
          this->partial_chunk.data.insert(this->partial_chunk.data.end(), new_data.begin(), new_data.end());
          this->remaining_chunk_size -= bytes_available;
          return bv.size();
        }

        const auto new_data = bv.subspan(pos, this->remaining_chunk_size);
        this->handler->on_data(new_data);
        this->partial_chunk.data.insert(this->partial_chunk.data.end(), new_data.begin(), new_data.end());
        pos += this->remaining_chunk_size;
        this->remaining_chunk_size = 0;

        this->internal_state = ChunkedDecoderState::READING_CHUNK_DATA_CR;
        break;
      }

      case ChunkedDecoderState::READING_TRAILER_SECTION: {
        if (!this->field_collection_parser.has_value() && bv[pos] == std::byte{'\r'}) {
          // No trailer section, we expect to see one final CRLF
          ++pos;
          this->internal_state = ChunkedDecoderState::READING_CHUNKED_BODY_LF;
          break;
        }

        if (!this->field_collection_parser.has_value()) {
          this->field_collection_parser.emplace();
        }

        const std::size_t consumed{this->field_collection_parser->feed(bv.subspan(pos))};

        switch (this->field_collection_parser->state()) {
          case FeedState::NEED_MORE_INPUT: {
            return bv.size();
          }

          case FeedState::COMPLETE: {
            this->handler->on_trailer_section(std::move(this->field_collection_parser->value()));
            this->field_collection_parser.reset();
            pos += consumed;
            this->internal_state = ChunkedDecoderState::READING_CHUNKED_BODY_CR;
            break;
          }

          case FeedState::ERROR: {
            this->field_collection_parser.reset();
            pos += consumed;
            this->internal_state = ChunkedDecoderState::INVALID;
            this->handler->on_error();
            break;
          }
        }

        break;
      }
    }
  }

  return pos;
}

FeedState ChunkedDecoder::state() const {
  switch (this->internal_state) {
    case ChunkedDecoderState::DONE: return FeedState::COMPLETE;
    case ChunkedDecoderState::INVALID: return FeedState::ERROR;
    default: return FeedState::NEED_MORE_INPUT;
  }
}

ChunkedDecoder::Handler ChunkedDecoder::default_handler{};

void ChunkedDecoder::set_handler(Handler & handler) {
  this->handler = &handler;
}

void ChunkedDecoder::unset_handler() {
  this->handler = &this->default_handler;
}

void ChunkedDecoder::Handler::on_data(std::span<const std::byte> data) {
  // Default implementation does nothing
}

void ChunkedDecoder::Handler::on_chunk(Chunk && chunk) {
  // Default implementation does nothing
}

void ChunkedDecoder::Handler::on_error() {
  // Default implementation does nothing
}

void ChunkedDecoder::Handler::on_trailer_section(FieldCollection && trailer_section) {
  // Default implementation does nothing
}

void ChunkedDecoder::Handler::on_finish() {
  // Default implementation does nothing
}
