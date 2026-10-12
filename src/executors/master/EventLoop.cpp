#include "EventLoop.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

EventLoop::EventLoop() { epoll_fd = epoll_create1(0); }
EventLoop::~EventLoop() { if (epoll_fd >= 0) close(epoll_fd); }


void EventLoop::register_connection(Connection * conn, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = conn;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn->get_fd(), &ev);
	active_connections[conn->get_fd()] = conn;
}

void EventLoop::modify_connection(Connection * conn, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = conn;
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->get_fd(), &ev);
}

void EventLoop::run_once(int timeout_ms) {
	struct epoll_event events[64];
	int n = epoll_wait(epoll_fd, events, 64, timeout_ms);

	for (int i = 0; i < n; i++) {
		Connection * conn = static_cast<Connection *>(events[i].data.ptr);
		uint32_t ev = events[i].events;

		if (ev & (EPOLLERR | EPOLLHUP)) {
			conn->on_error();
		} else {
			if (ev & EPOLLOUT) conn->on_write();
			if (ev & EPOLLIN) conn->on_read();
		}
	}
}
