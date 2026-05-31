#ifndef CHARACTER_CLASS_H
#define CHARACTER_CLASS_H

#include <array>
#include <cstdint>

const std::size_t NUM_BYTES{256};

class CharacterClass {
  public:
    CharacterClass(std::initializer_list<std::pair<std::uint8_t, std::uint8_t>> ranges);
    CharacterClass(std::initializer_list<std::uint8_t> chars);
    bool contains(std::uint8_t c) const;
  private:
    std::array<bool, NUM_BYTES> bytes{};
};

#endif
