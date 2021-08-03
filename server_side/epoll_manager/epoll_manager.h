#pragma once
#include<memory>
#include <sys/epoll.h>
#include<vector>
#include<string>
typedef struct epoll_event PollEvent;
struct EventPollPrivate;

class EventPoll {
	EventPollPrivate* m_private;

public:
	EventPoll();
	~EventPoll();
	void update_watch(int fd,std::string flag=std::string("all"),bool useET=true);
	void remove_watch(int fd);
	int event_wait(int timeout, std::vector<PollEvent>* ev);

};