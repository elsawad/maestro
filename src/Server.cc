#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include "Server.h"

Server::Server(const ServerConfig & config): config(config) {
  // Create accepting socket
  int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (sockfd == -1) {
    throw std::runtime_error("Error during socket creation");
  }
  this->sockfd = sockfd;

  // Creating epoll instance and registering accepting socket
  // int epfd = epoll_create1(0);
  // struct epoll_event ev;
  // ev.events = EPOLLIN;
  // ev.data.fd = sockfd;
  // if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
  //   throw std::runtime_error("Error adding socket to epoll");
  // }
  // this->epfd = epfd;

  this->addr.sin_family = AF_INET;
  this->addr.sin_port = htons(this->config.port);
  this->addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int bindStatus = bind(sockfd, (sockaddr *)&(this->addr), sizeof(this->addr));
  if (bindStatus < 0) {
    throw std::runtime_error("Error during socket binding");
  }

  int listenStatus = listen(sockfd, 20);
  if (listenStatus == -1) {
    throw std::runtime_error("Error during socket listening");
  }
}

Server::~Server() {
  close(this->epfd);
  close(this->sockfd);
}

uint16_t Server::getPort() const {
  sockaddr sa;
  socklen_t addr_len = sizeof(sa);
  if (getsockname(this->sockfd, &sa, &addr_len) == -1) {
    throw std::runtime_error("Error getting socket name");
  }
  return ntohs(((sockaddr_in *)&sa)->sin_port);
}

void Server::serve() {
  sockaddr_in in;
  socklen_t in_size = sizeof(in);
  int new_socket = accept4(this->sockfd, (struct sockaddr *)&in, &in_size, SOCK_NONBLOCK);
  if (new_socket < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return;
  }

  if (new_socket < 0) {
    std::ostringstream oss;
    oss << "Failed to accept incoming connection from "
      << inet_ntoa(in.sin_addr)
      << ", port "
      << ntohs(in.sin_port)
      << '\n';
    throw std::runtime_error(oss.str());
  }

  // Receipt of request
  HTTPRequest req;

  const size_t buf_size(4096);
  std::vector<uint8_t> buf(buf_size);

  ssize_t recv_ret;
  do {
    recv_ret = recv(new_socket, buf.data(), buf.size(), 0);

    if (recv_ret > 0) { 
      buf.resize(recv_ret);
      req.append(buf);
    }

    if (req.isValid()) {
      break;
    }

    // For stream-oriented sockets, recv_ret == 0 means the client has closed the connection
  } while (recv_ret > 0 || (recv_ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)));

  if (recv_ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
    throw std::runtime_error("recv failed");
  }

  std::cout << static_cast<int>(req.getMethod()) << " " << req.getRequestTarget() << " " << req.getHTTPVersion() << std::endl;

  // Transmission of response
  // HTTPResponse res(req);
  send(new_socket, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 37, 0);
  std::cerr << "Served request from " << inet_ntoa(in.sin_addr) << ", port " << ntohs(in.sin_port) << std::endl;
}
