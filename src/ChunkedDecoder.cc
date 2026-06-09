#include <algorithm>
#include <string>
#include <vector>

#include "ABNF.h"
#include "ChunkedDecoder.h"
#include "FeedResult.h"
#include "FieldCollectionParser.h"

// Feed a chunk of data to the decoder.
ChunkedDecoder::FeedResult ChunkedDecoder::feed(std::span<const std::byte> span) {
  std::size_t pos{0};
  while (pos < span.size() && this->status == Status::IN_PROGRESS) {
    switch(this->state) {
      case State::READING_CHUNK_SIZE: {
        while (pos < span.size() && HEXDIG.contains(span[pos])) {
          this->buffer.push_back(span[pos]);
          ++pos;
        }

        if (pos == span.size()) {
          // No non-HEXDIG character found, wait for more data
          return {this->status, span.last(0)};
        }

        // We found a non-HEXDIG character, so everything before this character is the chunk size in hexadecimal
        if (this->buffer.empty()) {
          // No chunk size found, invalid chunked encoding
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        const std::string chunk_size_str{reinterpret_cast<const char *>(this->buffer.data()), this->buffer.size()};
        this->buffer.clear();
        this->remaining_chunk_size = std::stoull(chunk_size_str, nullptr, 16);

        this->is_last_chunk = this->remaining_chunk_size == 0;
        this->state = State::READING_CHUNK_EXT;
        break;
      }

      case State::READING_CHUNK_EXT: {
        if (span[pos] == std::byte{'\r'}) {
          // No chunk extension, we expect to see a CRLF immediately after the chunk size
          ++pos;
          this->state = State::READING_CHUNK_SIZE_LF;
          break;
        }

        if (WSP.contains(span[pos])) {
          // There is a chunk extension and it begins with BWS
          ++pos;
          this->state = State::READING_CHUNK_EXT_SEMICOLON_BWS;
          break;
        }

        if (span[pos] == std::byte{';'}) {
          // There is a chunk extension and it begins with a semicolon
          ++pos;
          this->state = State::READING_CHUNK_EXT_NAME_BWS;
          break;
        }

        this->state = State::INVALID;
        this->handler->on_error();
        break;
      }

      case State::READING_CHUNK_EXT_SEMICOLON_BWS:
      case State::READING_CHUNK_EXT_NAME_BWS:
      case State::READING_CHUNK_EXT_REST_BWS:
      case State::READING_CHUNK_EXT_VAL_BWS: {
        while (pos < span.size() && WSP.contains(span[pos])) {
          ++pos;
        }

        if (pos == span.size()) {
          // We only found whitespace, wait for more data
          return {this->status, span.last(0)};
        }

        if (this->state == State::READING_CHUNK_EXT_SEMICOLON_BWS && span[pos] == std::byte{';'}) {
          ++pos;
          this->state = State::READING_CHUNK_EXT_NAME_BWS;
          break;
        }

        if (this->state == State::READING_CHUNK_EXT_NAME_BWS && tchar.contains(span[pos])) {
          this->state = State::READING_CHUNK_EXT_NAME;
          break;
        }

        if (this->state == State::READING_CHUNK_EXT_REST_BWS && span[pos] == std::byte{';'}) {
          // No extension value and extensions continue
          const std::string chunk_ext_name{*this->chunk_ext_name_parser->value()};
          this->chunk_ext_name_parser.reset();

          this->partial_chunk.extensions.emplace_back(chunk_ext_name, std::nullopt);

          ++pos;
          this->state = State::READING_CHUNK_EXT_NAME_BWS;
          break;
        }

        if (this->state == State::READING_CHUNK_EXT_REST_BWS && span[pos] == std::byte{'='}) {
          // There is an extension value
          ++pos;
          this->state = State::READING_CHUNK_EXT_VAL_BWS;
          break;
        }

        if (this->state == State::READING_CHUNK_EXT_REST_BWS && span[pos] == std::byte{'\r'}) {
          // No chunk extension value and extensions end, we expect to see a CRLF immediately after the chunk extension name
          const std::string chunk_ext_name{*this->chunk_ext_name_parser->value()};
          this->chunk_ext_name_parser.reset();

          this->partial_chunk.extensions.emplace_back(chunk_ext_name, std::nullopt);

          ++pos;
          this->state = State::READING_CHUNK_SIZE_LF;
          break;
        }

        if (this->state == State::READING_CHUNK_EXT_VAL_BWS && (tchar.contains(span[pos]) || span[pos] == std::byte{'"'})) {
          this->state = State::READING_CHUNK_EXT_VAL;
          break;
        }

        this->state = State::INVALID;
        this->handler->on_error();
        break;
      }

      case State::READING_CHUNK_EXT_NAME: {
        if (!this->chunk_ext_name_parser.has_value()) {
          this->chunk_ext_name_parser.emplace();
        }

        const auto [status, consumed] = this->chunk_ext_name_parser->feed(span.subspan(pos));

        if (status == FeedState::NEED_MORE_INPUT) {
          return {this->status, span.last(0)};
        }

        if (status == FeedState::ERROR) {
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        pos += consumed;
        this->state = State::READING_CHUNK_EXT_REST_BWS;
        break;
      }

      case State::READING_CHUNK_EXT_VAL: {
        if (!this->chunk_ext_val_parser.has_value() && tchar.contains(span[pos])) {
          this->chunk_ext_val_parser.emplace(std::in_place_type<TokenParser>);
        } else if (!this->chunk_ext_val_parser.has_value() && span[pos] == std::byte{'"'}) {
          this->chunk_ext_val_parser.emplace(std::in_place_type<QuotedStringParser>);
        } else if (!this->chunk_ext_val_parser.has_value()) {
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        const auto [status, consumed] = std::visit([span, pos](auto & parser) {
          return parser.feed(span.subspan(pos));
        }, *this->chunk_ext_val_parser);

        if (status == FeedState::NEED_MORE_INPUT) {
          return {this->status, span.last(0)};
        }

        if (status == FeedState::ERROR) {
          this->state = State::INVALID;
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
        this->state = State::READING_CHUNK_EXT;
        break;
      }

      case State::READING_CHUNK_DATA_CR:
      case State::READING_CHUNKED_BODY_CR: {
        if (span[pos] != std::byte{'\r'}) {
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        ++pos;
        this->state =
            this->state == State::READING_CHUNK_DATA_CR ? State::READING_CHUNK_DATA_LF
          : State::READING_CHUNKED_BODY_LF;
        break;
      }

      case State::READING_CHUNK_SIZE_LF:
      case State::READING_CHUNK_DATA_LF:
      case State::READING_CHUNKED_BODY_LF: {
        if (span[pos] != std::byte{'\n'}) {
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        if ((this->state == State::READING_CHUNK_DATA_LF && !this->is_last_chunk) || (this->state == State::READING_CHUNK_SIZE_LF && this->is_last_chunk)) {
          this->handler->on_chunk(std::move(this->partial_chunk));
        }

        if (this->state == State::READING_CHUNK_DATA_LF && !this->is_last_chunk) {
          this->partial_chunk = Chunk{};
        }

        ++pos;
        this->state =
            this->state == State::READING_CHUNK_SIZE_LF && this->is_last_chunk ? State::READING_TRAILER_SECTION
          : this->state == State::READING_CHUNK_SIZE_LF && !this->is_last_chunk ? State::READING_CHUNK_DATA
          : this->state == State::READING_CHUNK_DATA_LF ? State::READING_CHUNK_SIZE
          : State::DONE;

        if (this->state == State::DONE) {
          this->handler->on_finish();
        }

        break;
      }

      case State::READING_CHUNK_DATA: {
        const std::size_t bytes_available = span.size() - pos;

        if (bytes_available == 0) {
          return {this->status, span.last(0)};
        }

        if (bytes_available < this->remaining_chunk_size) {
          const auto new_data = span.subspan(pos);
          this->handler->on_data(new_data);
          this->partial_chunk.data.insert(this->partial_chunk.data.end(), new_data.begin(), new_data.end());
          this->remaining_chunk_size -= bytes_available;
          return {this->status, span.last(0)};
        }

        const auto new_data = span.subspan(pos, this->remaining_chunk_size);
        this->handler->on_data(new_data);
        this->partial_chunk.data.insert(this->partial_chunk.data.end(), new_data.begin(), new_data.end());
        pos += this->remaining_chunk_size;
        this->remaining_chunk_size = 0;

        this->state = State::READING_CHUNK_DATA_CR;
        break;
      }

      case State::READING_TRAILER_SECTION: {
        if (!this->field_collection_parser.has_value() && span[pos] == std::byte{'\r'}) {
          // No trailer section, we expect to see one final CRLF
          ++pos;
          this->state = State::READING_CHUNKED_BODY_LF;
          break;
        }

        if (!this->field_collection_parser.has_value()) {
          this->field_collection_parser.emplace();
        }

        const auto [status, remaining] = this->field_collection_parser->feed(span.subspan(pos));

        switch (status) {
          case FieldCollectionParser::Status::IN_PROGRESS: {
            return {this->status, span.last(0)};
          }

          case FieldCollectionParser::Status::DONE: {
            this->handler->on_trailer_section(std::move(this->field_collection_parser->value()));
            pos = span.size() - remaining.size();
            this->state = State::READING_CHUNKED_BODY_CR;
            break;
          }

          case FieldCollectionParser::Status::INVALID: {
            this->state = State::INVALID;
            this->handler->on_error();
            break;
          }
        }

        break;
      }

      case State::INVALID: {
        this->status = Status::INVALID;
        break;
      }

      case State::DONE: {
        this->status = Status::DONE;
        break;
      }
    }
  }

  if (this->state == State::DONE) {
    this->status = Status::DONE;
  } else if (this->state == State::INVALID) {
    this->status = Status::INVALID;
  }

  return {this->status, span.subspan(pos)};
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
