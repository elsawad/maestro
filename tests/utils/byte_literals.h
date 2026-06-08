#ifndef BYTE_LITERALS_H
#define BYTE_LITERALS_H

#include <vector>

std::vector<std::byte> operator"" _b(const char *str, std::size_t len);

#endif
