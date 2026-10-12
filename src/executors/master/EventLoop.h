#include "Connection.h"
#include <unordered_map>

class EventLoop {
public:
	EventLoop();
	~EventLoop();

	void register_connection(Connection * conn, uint32_t events);
	void modify_connection(Connection * conn, uint32_t events);
	void run_once(int timeout_ms);
private:
	int epoll_fd;
	std::unordered_map<int, Connection *> active_connections;
};
