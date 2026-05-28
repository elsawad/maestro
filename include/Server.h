#ifndef SERVER_H
#define SERVER_H

#include <filesystem>
#include <netinet/in.h>
#include <string>

class Options;

class Server {
  private:
    const std::filesystem::path root;
    sockaddr_in addr;
    int sockfd;
    int epfd;
  public:
    Server(Options const &options);
    ~Server();
    uint16_t getPort() const;
    void serve();
};

#endif
