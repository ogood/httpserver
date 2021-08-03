#pragma once
#include<string>
#include<stdint.h>
#include<future>
#include<list>
#include<unordered_map>
#include<functional>
#include <atomic>
#include<mutex>
#include <server_side/epoll_manager/epoll_manager.h>
#include <server_side/tcp_service/thread_pool.h>

class EventPoll;
class TCPService {
protected:

	using Reply_CB = std::function<std::pair<char*, int>(int)>;
	static std::list<TCPService*>  _running_inst;
	std::unordered_map<int, Reply_CB> pendingsend;
	std::mutex _lock;
	uint16_t _port;
	EventPoll* _epoll;
	static const int _max_sockt = 10;// socket queue
	static const int _max_bytes = 2048;//max bytes to recieve once.
	int _socket;
	std::future<void> _waiting;// loop socket client connection

	ThreadPool* _threads;



	void loop_handler(int ev);
	void client_handler(int fd, int ev);

	virtual int request_in(char* in, int insize, Reply_CB*)=0;

public:
	void close_socket(int fd);
	TCPService(int port=8080);
	~TCPService();
	void start();
	void loop();
	static void stop(TCPService* p = nullptr);
};