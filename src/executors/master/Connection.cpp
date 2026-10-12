#include "Connection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cerrno>

Connection::Connection(const std::string& ip, int port)
    : ip(ip), port(port), sockfd(-1) {}

Connection::~Connection() {
    close_socket();
}

bool Connection::connect_non_blocking() {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) return false;

	int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &target_addr.sin_addr);

    int res = connect(sockfd, (struct sockaddr*)&target_addr, sizeof(target_addr));
    if (res < 0 && errno != EINPROGRESS) {
        close_socket();
        return false;
    }
    return true;
}

#include <poll.h>

bool Connection::flush() {
    if (sockfd < 0) return false;
    while (!write_buffer.empty()) {
        ssize_t sent = send(sockfd, write_buffer.c_str(), write_buffer.length(), 0);
        if (sent > 0) {
            write_buffer.erase(0, sent);
        } else if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = sockfd;
                pfd.events = POLLOUT;
                int ret = poll(&pfd, 1, 10000);
                if (ret <= 0) return false;
            } else {
                return false;
            }
        }
    }
    return true;
}

bool Connection::send_data(const std::string& data) {
    if (sockfd < 0) return false;
	write_buffer += data;
	return flush();
}

ssize_t Connection::receive_data(std::string& out_buf) {
    if (sockfd < 0) return -1;
	
	char buf[2048];
	memset(buf, 0, sizeof(buf));
	ssize_t bytes = recv(sockfd, buf, sizeof(buf) - 1, 0);

	if (bytes > 0)
		out_buf.append(buf, bytes);
	
	return bytes;
}

void Connection::close_socket() {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}
