#include "Options.h"

#include <exception>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

Options::Options(int const argc, char const * const * const argv) {
  std::string_view svroot;
  std::string_view svport;

  int i(1);
  while (i < argc) {
    std::string_view option(argv[i]);
    if (option == "-r" && i + 1 < argc) {
      svroot = argv[i + 1];
      i += 2;
    } else if (option == "-p" && i + 1 < argc) {
      svport = argv[i + 1];
      i += 2;
    } else {
      throw std::invalid_argument("invalid argument " + std::string(option));
    }
  }

  // check root directory
  if (svroot.empty()) {
    throw std::invalid_argument("root is a required argument");
  }

  std::filesystem::path proot(svroot);
  if (!std::filesystem::is_directory(proot)) {
    throw std::invalid_argument("root is not a directory");
  }

  // check port number
  int iport;
  if (svport.empty()) {
    iport = 0; // dynamic port assignment
  } else {
    try {
      iport = std::stoi(svport.data());
    } catch (std::invalid_argument &e) {
      std::throw_with_nested(std::invalid_argument("port must be a number"));
    } catch (std::out_of_range &e) {
      std::throw_with_nested(std::out_of_range("port must be between 0-65535"));
    }

    if (iport < std::numeric_limits<uint16_t>::min() || iport > std::numeric_limits<uint16_t>::max()) {
      throw std::out_of_range("port must be between 0-65535");
    }
  }

  this->root = std::move(proot);
  this->port = std::move(iport);
}

uint16_t Options::getPort() const {
  return this->port;
}

std::filesystem::path const & Options::getRoot() const {
  return this->root;
}
