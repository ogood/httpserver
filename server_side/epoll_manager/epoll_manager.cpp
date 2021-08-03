#include "epoll_manager.h"
#include<memory>
#include <stdlib.h>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include<unistd.h>
#include<set>
#include <vector>
#include<common/utility.h>


//typedef struct epoll_event EpollEvent;
struct EventPollPrivate {
	static const int max_event=10;
	int epoll_fd;
	std::set<int> added_fd;
	//std::vector<PollEvent_t> ready_events;
};
EventPoll::EventPoll()
{
	m_private = new EventPollPrivate;
	
	m_private->epoll_fd = epoll_create1(0);
	if (m_private->epoll_fd == -1)
		std::terminate();

}



EventPoll::~EventPoll()
{
	
	delete m_private;
}

void EventPoll::update_watch(int fd_id, std::string flag, bool useET)
{
	PollEvent ev;
	if (flag == "all") {
		ev.events = EPOLLIN | EPOLLPRI | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
	}
	else {
		ev.events = 0;
		if (flag.find('r') != std::string::npos)
			ev.events |= EPOLLIN | EPOLLPRI;//ready to read or read urgent msg
		if (flag.find('w') != std::string::npos)
			ev.events |= EPOLLOUT;
		if (flag.find('s') != std::string::npos)
			ev.events |= EPOLLHUP | EPOLLRDHUP;//local close or remote close
		if (flag.find('e') != std::string::npos)
			ev.events |= EPOLLERR;// write closed socket
	}
	if (useET)
		ev.events |= EPOLLET;
	ev.data.fd = fd_id;
	auto f=m_private->added_fd.find(fd_id);
	if (f == m_private->added_fd.end()) {
		
		if (epoll_ctl(m_private->epoll_fd, EPOLL_CTL_ADD, fd_id, &ev))
		{
			LOG(ERROR) << "failed to add fd into epoll.";
			return;
		}
		m_private->added_fd.insert(fd_id);
	}
	else {
		if (epoll_ctl(m_private->epoll_fd, EPOLL_CTL_MOD, fd_id, &ev))
		{
			LOG(ERROR) << "failed to modify fd in epoll.";
			return;
		}
	}

}
void EventPoll::remove_watch(int fd)
{
	auto f = m_private->added_fd.find(fd);
	if (f == m_private->added_fd.end()) {
		return;
	}
	if (epoll_ctl(m_private->epoll_fd, EPOLL_CTL_DEL, fd, NULL))
	{
		LOG(ERROR) << "failed to delete fd in epoll.";
		return;
	}
	m_private->added_fd.erase(f);
}
//等待的毫秒数，-1代表无限等待
int EventPoll::event_wait(int timeout, std::vector<PollEvent>* fds)
{
	if (!fds) {
		LOG(FATAL) << "event_wait vector container can't be nullptr";
	}

	return epoll_wait(m_private->epoll_fd, fds->data(), fds->size(), timeout);

}
