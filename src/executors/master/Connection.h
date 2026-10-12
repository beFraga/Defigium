#pragma once

#include <string>
#include <netinet/in.h>

class Connection {
	public:
		Connection(const std::string& ip, int port);
		~Connection();

		bool connect_non_blocking();
		void close_socket();

		bool send_data(const std::string& data);
		bool flush();
		ssize_t receive_data(std::string& out_buf);

		virtual void on_read() = 0;
		virtual void on_write() = 0;
		virtual void on_error() = 0;

		std::string get_ip() const { return ip; }
		int get_fd() const { return sockfd; }
		int get_port() const { return port; }
	private:
		std::string ip;
		int port;
	protected:
		int sockfd;
		std::string read_buffer, write_buffer;
};
