#include <algorithm>
#include <span>

#include "ABNF.h"
#include "Field.h"
#include "FieldCollectionParser.h"
#include "TokenParser.h"

const CharacterClass field_vchar{{0x21, 0x7E}, {0x80, 0xFF}}; // VCHAR / obs-text

FieldCollectionParser::FeedResult FieldCollectionParser::feed(std::span<const std::byte> span) {
  size_t pos = 0;

  while (pos < span.size() && this->status == Status::IN_PROGRESS) {
    switch (this->state) {
      case State::READING_FIELD_NAME: {
        if (this->field_name.empty() && span[pos] == std::byte{'\r'}) {
          this->state = State::READING_FINAL_CR;
          break;
        }

        std::uint8_t c;
        while (pos < span.size() && tchar.contains(c = static_cast<std::uint8_t>(span[pos]))) {
          this->field_name.push_back(c);
          ++pos;
        }

        if (pos == span.size()) {
          return {this->status, span.subspan(pos)};
        }

        if (this->field_name.empty() || span[pos] != std::byte{':'}) {
          this->state = State::INVALID;
          break;
        }

        ++pos;
        this->state = State::READING_OWS_BEFORE_FIELD_VALUE;
        break;
      }

      case State::READING_OWS_BEFORE_FIELD_VALUE: {
        while (pos < span.size() && WSP.contains(static_cast<std::uint8_t>(span[pos]))) {
          ++pos;
        }

        if (pos == span.size()) {
          return {this->status, span.subspan(pos)};
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

        // For simplicity, we will look for any character that is not in field-vchar or SP/HTAB.
        std::uint8_t c;
        while (pos < span.size() && (field_vchar.contains(c = static_cast<std::uint8_t>(span[pos])) || WSP.contains(c))) {
          this->field_value.push_back(c);
          ++pos;
        }

        if (pos == span.size()) {
          return {this->status, span.subspan(pos)};
        }

        // We will clear out optional whitespace at the end of the field value and move on to the CRLF reading phase.
        auto rit{this->field_value.crbegin()};
        while (rit != this->field_value.crend() && WSP.contains(static_cast<std::uint8_t>(*rit))) {
          ++rit;
        }

        if (rit == this->field_value.crend()) {
          // No field-vchar found at the iterator position
          this->state = State::INVALID;
          break;
        }

        this->field_value.erase(rit.base(), this->field_value.cend());
        this->state = State::READING_FIELD_LINE_CR;
        break;
      }

      case State::READING_FIELD_LINE_CR:
      case State::READING_FINAL_CR: {
        if (span[pos] != std::byte{'\r'}) {
          // No CR found at the iterator position
          this->state = State::INVALID;
          break;
        }

        if (this->state == State::READING_FIELD_LINE_CR) {
          this->state = State::READING_FIELD_LINE_LF;
        } else {
          this->state = State::READING_FINAL_LF;
        }

        ++pos;
        break;
      }

      case State::READING_FIELD_LINE_LF:
      case State::READING_FINAL_LF: {
        if (span[pos] != std::byte{'\n'}) {
          // No LF found at the iterator position
          this->state = State::INVALID;
          break;
        }

        if (this->state == State::READING_FIELD_LINE_LF) {
          this->field_collection.emplace_back(std::move(this->field_name), std::move(this->field_value));
          this->field_name.clear();
          this->field_value.clear();

          this->state = State::READING_FIELD_NAME;
        } else {
          this->state = State::DONE;
        }

        ++pos;
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

FieldCollection & FieldCollectionParser::get_fields() {
  return this->field_collection;
}
