#ifndef OPTIONS_H
#define OPTIONS_H

#include <filesystem>
#include <string_view>

class Options {
  public:
    Options(int const argc, char const * const * const argv);
    std::filesystem::path const & getRoot() const;
    uint16_t getPort() const;
  private:
    std::filesystem::path root;
    uint16_t port;
};

#endif
