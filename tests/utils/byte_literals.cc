#include "byte_literals.h"

std::vector<std::byte> operator"" _b(const char *str, std::size_t len) {
  std::vector<std::byte> result;
  result.reserve(len);
  for (std::size_t i = 0; i < len; ++i) {
    result.push_back(static_cast<std::byte>(str[i]));
  }
  return result;
}
