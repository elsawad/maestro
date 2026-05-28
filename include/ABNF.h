#ifndef ABNF_H
#define ABNF_H

#include "CharacterClass.h"

// ABNF basic rules rules as defined in RFC 5234 appendix B.1
extern const CharacterClass HEXDIG;
extern const CharacterClass DIGIT;
extern const CharacterClass TCHAR;
extern const CharacterClass VCHAR;
extern const CharacterClass WSP;

#endif
