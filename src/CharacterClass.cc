#include <stdexcept>

#include "CharacterClass.h"

CharacterClass::CharacterClass(std::initializer_list<std::pair<std::uint8_t, std::uint8_t>> ranges) {
  for (const auto &[start, end] : ranges) {
    if (start > end) {
      throw std::invalid_argument("Invalid character range");
    }
    for (std::uint8_t c = start; c < end; ++c) {
      this->bytes[c] = true;
    }
    this->bytes[end] = true;
  }
}

CharacterClass::CharacterClass(std::initializer_list<std::uint8_t> chars) {
  for (const auto &c : chars) {
    this->bytes[c] = true;
  }
}

bool CharacterClass::contains(std::byte c) const noexcept {
  return this->bytes[std::to_integer<std::size_t>(c)];
}

bool CharacterClass::contains(std::uint8_t c) const noexcept {
  return this->bytes[c];
}
