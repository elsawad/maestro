#ifndef SERVER_CONFIG_CLI_PARSER_H
#define SERVER_CONFIG_CLI_PARSER_H

#include "ServerConfigParserError.h"

struct ServerConfigCLIParser {
  static ServerConfigParserResult parse(int argc, const char * const * argv);
};

#endif
