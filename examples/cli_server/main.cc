#include <iostream>

#include "Server.h"
#include "ServerConfigBuilder.h"
#include "ServerConfigCLIParser.h"
#include "ServerConfigPatch.h"

int main(int argc, const char * const * argv) {
  ServerConfigParserResult server_config_parser_result{ServerConfigCLIParser::parse(argc, argv)};
  if (!server_config_parser_result.has_value()) {
    std::cerr << "error: " << server_config_parser_result.error().message << std::endl;
    return 1;
  }

  ServerConfigPatch server_config_patch{*server_config_parser_result};
  ServerConfigBuilder server_config_builder{};
  server_config_builder.patch(std::move(server_config_patch));

  ServerConfigBuilderResult server_config_builder_result{std::move(server_config_builder).build()};
  if (!server_config_builder_result.has_value()) {
    std::cerr << "error: " << server_config_builder_result.error().message << std::endl;
    return 1;
  }

  ServerConfig server_config{*server_config_builder_result};
  Server server{server_config};
  while(true) {
    server.serve();
  }
}
