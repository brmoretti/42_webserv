#include "Server.hpp"

void Server::_initServerAddress(sockaddr_in &server_add)
{
	memset(&server_add, 0, sizeof(server_add));
	server_add.sin_family = AF_INET;
	server_add.sin_addr.s_addr = inet_addr(_address.c_str());
	server_add.sin_port = htons(_port);
	if (bind(_server_fd, (sockaddr *)&server_add, sizeof(server_add)) == -1)
		throw std::runtime_error("Failed to bind socket");
	if (listen(_server_fd, MAX_CONNECTIONS) == -1)
		throw std::runtime_error("Failed to listen socket");
}

void Server::_initServer()
{
	this->_server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	OUTNL("SERVER FD: " << this->_server_fd);
	if (this->_server_fd == -1)
		throw std::runtime_error("Failed to create socket");
	int opt = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
		throw std::runtime_error("Failed to set socket options");
	sockaddr_in server_addr;
	this->_initServerAddress(server_addr);
	(*_event).data.fd = this->_server_fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, _event) == -1)
		throw std::runtime_error("Failed to add server socket to epoll");
}
