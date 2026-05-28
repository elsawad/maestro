#include "ABNF.h"

const CharacterClass DIGIT{{0x30, 0x39}};
const CharacterClass HEXDIG{{0x30, 0x39}, {'A', 'F'}};
const CharacterClass VCHAR{{0x21, 0x7E}};
const CharacterClass WSP{' ', '\t'};
