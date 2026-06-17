#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#include "ServerConfig.h"

class Server {
  private:
    ServerConfig config;
    sockaddr_in addr;
    int sockfd;
    int epfd;
  public:
    Server(const ServerConfig & config);
    ~Server();
    uint16_t getPort() const;
    void serve();
};

#endif
