#include "ABNF.h"
#include "FieldCollectionParser.h"

const CharacterClass field_vchar{{0x21, 0x7E}, {0x80, 0xFF}}; // VCHAR / obs-text

FieldCollectionParser::FeedResult FieldCollectionParser::feed(std::span<const std::byte> span) {
  std::size_t pos{0};
  while (pos < span.size() && this->status == Status::IN_PROGRESS) {
    switch (this->state) {
      case State::READING_FIELD_NAME: {
        if (!this->field_name_parser.has_value() && !tchar.contains(span[pos])) {
          this->state = State::DONE;
          break;
        }

        if (!this->field_name_parser.has_value()) {
          this->field_name_parser.emplace();
        }

        const auto [status, consumed] = this->field_name_parser->feed(span.subspan(pos));

        switch (status) {
          case FeedState::NEED_MORE_INPUT: {
            return {this->status, span.last(0)};
          }

          case FeedState::COMPLETE: {
            this->field_name = std::move(*this->field_name_parser->value());
            this->field_name_parser.reset();

            pos += consumed;
            // TokenParser must see a non-tchar to end a token, so pos + consumed < span.size()
            if (span[pos] != std::byte{':'}) {
              this->state = State::INVALID;
              break;
            }

            ++pos;
            this->state = State::READING_OWS_BEFORE_FIELD_VALUE;
            break;
          }

          case FeedState::ERROR: {
            this->status = Status::INVALID;
            break;
          }
        }

        break;
      }

      case State::READING_OWS_BEFORE_FIELD_VALUE: {
        while (pos < span.size() && WSP.contains(span[pos])) {
          ++pos;
        }

        if (pos == span.size()) {
          return {this->status, span.last(0)};
        }

        this->state = State::READING_FIELD_VALUE;
        break;
      }

      case State::READING_FIELD_VALUE: {
        // field-value    = *field-content
        // field-content  = field-vchar
        //                 [ 1*( SP / HTAB / field-vchar ) field-vchar ]
        // field-vchar    = VCHAR / obs-text
        // obs-text       = %x80-FF

        // For simplicity, we will look for any character that is not in field-vchar or WSP.
        std::byte c;
        while (pos < span.size() && (field_vchar.contains(c = span[pos]) || WSP.contains(c))) {
          this->field_value.push_back(static_cast<char>(c));
          ++pos;
        }

        if (pos == span.size()) {
          return {this->status, span.last(0)};
        }

        if (c != std::byte{'\r'}) {
          this->state = State::INVALID;
          break;
        }

        // We will clear out optional whitespace at the end of the field value and move on to the CRLF reading phase.
        auto rit{this->field_value.crbegin()};
        while (WSP.contains(*rit)) {
          ++rit;
        }
        this->field_value.erase(rit.base(), this->field_value.cend());

        ++pos;
        this->state = State::READING_FIELD_LINE_LF;
        break;
      }

      case State::READING_FIELD_LINE_LF: {
        if (span[pos] != std::byte{'\n'}) {
          // No LF found at the iterator position
          this->state = State::INVALID;
          break;
        }

        this->field_collection.emplace_back(std::move(this->field_name), std::move(this->field_value));
        this->field_name.clear();
        this->field_value.clear();

        ++pos;
        this->state = State::READING_FIELD_NAME;
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

  return {this->status, span.subspan(pos)};
}

FieldCollection & FieldCollectionParser::value() {
  return this->field_collection;
}
