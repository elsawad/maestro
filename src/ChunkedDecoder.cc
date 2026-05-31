#include <algorithm>
#include <string>
#include <vector>

#include "ABNF.h"
#include "ChunkedDecoder.h"
#include "FieldCollectionParser.h"

// Feed a chunk of data to the decoder.
ChunkedDecoder::FeedResult ChunkedDecoder::feed(std::span<const std::byte> span) {
  std::size_t pos{0};
  while (pos < span.size() && this->status == Status::IN_PROGRESS) {
    switch(this->state) {
      case State::READING_CHUNK_SIZE: {
        while (pos < span.size() && HEXDIG.contains(static_cast<std::uint8_t>(span[pos]))) {
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

        const std::string current_chunk_size_str{reinterpret_cast<const char *>(this->buffer.data()), this->buffer.size()};
        this->buffer.clear();
        this->current_chunk_size = std::stoull(current_chunk_size_str, nullptr, 16);

        if (this->current_chunk_size == 0) {
          this->state = State::READING_LAST_CHUNK_EXT;
        } else {
          this->state = State::READING_CHUNK_EXT;
        }

        break;
      }

      case State::READING_CHUNK_EXT:
      case State::READING_LAST_CHUNK_EXT: {
        // We will ignore chunk extensions here since they are optional and not commonly used, but we will still need to read until the CRLF at the end of the chunk size line
        const auto extensions_start{span.begin() + pos};
        auto extensions_end = std::find(extensions_start, span.end(), std::byte{'\r'});
        pos += std::distance(extensions_start, extensions_end);
        if (extensions_end == span.end()) {
          // No CRLF found, wait for more data
          return {this->status, span.last(0)};
        }

        this->state =
          this->state == State::READING_CHUNK_EXT ? State::READING_CHUNK_SIZE_CR
          : State::READING_LAST_CHUNK_CR;
        break;
      }

      case State::READING_CHUNK_SIZE_CR:
      case State::READING_CHUNK_DATA_CR:
      case State::READING_LAST_CHUNK_CR:
      case State::READING_CHUNKED_BODY_CR: {
        if (span[pos] != std::byte{'\r'}) {
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        ++pos;
        this->state =
            this->state == State::READING_CHUNK_SIZE_CR ? State::READING_CHUNK_SIZE_LF
          : this->state == State::READING_CHUNK_DATA_CR ? State::READING_CHUNK_DATA_LF
          : this->state == State::READING_LAST_CHUNK_CR ? State::READING_LAST_CHUNK_LF
          : State::READING_CHUNKED_BODY_LF;
        break;
      }

      case State::READING_CHUNK_SIZE_LF:
      case State::READING_CHUNK_DATA_LF:
      case State::READING_LAST_CHUNK_LF:
      case State::READING_CHUNKED_BODY_LF: {
        if (span[pos] != std::byte{'\n'}) {
          this->state = State::INVALID;
          this->handler->on_error();
          break;
        }

        if (this->state == State::READING_CHUNK_DATA_LF || this->state == State::READING_LAST_CHUNK_LF) {
          this->handler->on_chunk(std::move(this->partial_chunk));
        }

        if (this->state == State::READING_CHUNK_DATA_LF) {
          this->partial_chunk = Chunk{};
        }

        ++pos;
        this->state =
            this->state == State::READING_CHUNK_SIZE_LF ? State::READING_CHUNK_DATA
          : this->state == State::READING_CHUNK_DATA_LF ? State::READING_CHUNK_SIZE
          : this->state == State::READING_LAST_CHUNK_LF ? State::READING_TRAILER_SECTION
          : State::DONE;

        if (this->state == State::DONE) {
          this->handler->on_finish();
        }

        break;
      }

      case State::READING_CHUNK_DATA: {
        const auto cur = span.begin() + pos;
        const std::ptrdiff_t bytes_available = std::distance(cur, span.end());
        if (bytes_available < this->current_chunk_size) {
          const auto new_data = std::vector<std::byte>{cur, span.end()};
          this->handler->on_data(new_data);
          this->partial_chunk.data.insert(this->partial_chunk.data.end(), new_data.begin(), new_data.end());
          this->current_chunk_size -= bytes_available;
          return {this->status, span.last(0)};
        }

        const auto new_data = std::vector<std::byte>{cur, cur + this->current_chunk_size};
        this->handler->on_data(new_data);
        this->partial_chunk.data.insert(this->partial_chunk.data.end(), new_data.begin(), new_data.end());
        pos += this->current_chunk_size;
        this->current_chunk_size = 0;

        this->state = State::READING_CHUNK_DATA_CR;
        break;
      }

      case State::READING_TRAILER_SECTION: {
        if (!this->field_collection_parser.has_value() && span[pos] == std::byte{'\r'}) {
          // No trailer section, we expect to see one final CRLF
          this->state = State::READING_CHUNKED_BODY_CR;
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
            this->handler->on_trailer_section(std::move(this->field_collection_parser->get_fields()));
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

ChunkedDecoder::DefaultHandler ChunkedDecoder::default_handler{};

void ChunkedDecoder::set_handler(Handler & handler) {
  this->handler = &handler;
}

void ChunkedDecoder::unset_handler() {
  this->handler = &this->default_handler;
}

void ChunkedDecoder::Handler::on_data(const std::vector<std::byte> & data) {
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
