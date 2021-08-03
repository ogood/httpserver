#include "tcp_service.h"
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <sys/socket.h>

#include<fcntl.h>
#include<memory>
#include<common/utility.h>
//#include"http.h"
#include <sys/epoll.h>

std::list<TCPService*>  TCPService::_running_inst ;

void TCPService::close_socket(int fd)
{
    DLOG(INFO) << "to shutdown sfd:" << fd << ", and remove from epoll";
    shutdown(fd, SHUT_RDWR);
    _epoll->remove_watch(fd);
}

TCPService::TCPService(int port) :_port(port)
{

}

TCPService::~TCPService()
{
    _waiting.wait();
    delete _threads;

}

void TCPService::start()
{

    _socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in svr_addr;

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = htons(INADDR_ANY);
    svr_addr.sin_port = htons(_port);
    int optval = 1;
    setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (bind(_socket, (struct sockaddr*)&svr_addr, sizeof(svr_addr)))
    {
        LOG(FATAL) << "failed to bind  port:" << _port;
    }
    if (listen(_socket, _max_sockt)) {
        LOG(FATAL) << "failed to listen port:" << _port;
   }
    fcntl(_socket, F_SETFL, O_NONBLOCK);
    LOG(INFO) << "tcp socket fd:" << _socket << ",on port:" << _port<<"\n";
    _epoll = new EventPoll;
    _epoll->update_watch(_socket, "all");
    _running_inst.push_back(this);
    //工作线程
    _threads = new ThreadPool(4);
    //循环查询线程
   _waiting=std::async(std::launch::async, &TCPService::loop, this);
   _waiting.wait();
    
}

void TCPService::loop()
{
    std::vector<PollEvent> events(10);
    while (true) {
        auto cnt = _epoll->event_wait(-1,&events);
        DLOG_IF(INFO, cnt > 0) << "epoll occured. ready events cnt:" << cnt;

        for (int i = 0; i < cnt; i++) {
            if (events[i].data.fd == _socket)
                loop_handler(events[i].events);
            else {
                DLOG(INFO) << "To push task , fd:" << events[i].data.fd << ", event type:"<<events[i].events;
                _threads->pushVoidTask(&TCPService::client_handler, this, events[i].data.fd, events[i].events);
            }
              
        }
    }


}

void TCPService::stop(TCPService* p )
{
    if (p) {
        shutdown(p->_socket, SHUT_RDWR);
        delete p;
        _running_inst.remove(p);
    }
    else {
        for (auto i : _running_inst) {
            shutdown(p->_socket, SHUT_RDWR);
            delete i;
        }
        _running_inst.clear();
    }
}

void TCPService::loop_handler(int ev)
{ 
    if (ev & EPOLLIN)
    {
        struct sockaddr_in clt_addr;
        socklen_t clt_len = sizeof(clt_addr);
        int listenfd;
        while (true) {
                listenfd = accept(_socket, (struct sockaddr*)&clt_addr, &clt_len);

                if (listenfd <0 )
                    break;

                std::string ip;
                ip.resize(16);
                inet_ntop(AF_INET, &clt_addr.sin_addr ,&ip[0],  16);
                DLOG(INFO) << "\n\n=======client connected==============\nclient ip:" << ip
                << " client port:" << ntohs(clt_addr.sin_port)<<", fd:"<< listenfd<<"\n";

                fcntl(listenfd, F_SETFL, O_NONBLOCK);
                _epoll->update_watch(listenfd, "rse");//read shutdown,error
        }
 

        return;
    }
    else   if (ev & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        close_socket(_socket);
    }else
    DLOG(INFO) << "Un handled fd:" << _socket << ", event:" << ev;
}
//由线程池负责consume
void TCPService::client_handler(int fd,int ev)
{

    if (ev & 8192) {//client 意外断开 1<<13==8192
        DLOG(INFO) << "remote disconnected. close socket fd:"<<fd;
        close_socket(fd);
        return;
    }
    if (ev & (EPOLLIN | EPOLLPRI))//to read
    {
        std::string buf;
        int len,total=0;
        buf.resize(_max_bytes);
 
        while ((len = read(fd, (char*)&buf[total], buf.size()-total)) > 0) {
            total += len;
        }
        DLOG(INFO) << "client sended data length:" << total;// << " content:\n" << buf;
        if (errno != EAGAIN){

               DLOG(INFO) << "Error!= EAGAIN, too much bytes?";
               std::string b;
               b.resize(100);
               sprintf((char*)&b[0], "Error,too much bytes? max:%d\n", _max_bytes);
               write(fd, (char*)&b[0], b.find("\n")+1);
               close_socket(fd);
        }

        Reply_CB fun;
       if( request_in(&buf.front(), total, &fun))
           close_socket(fd);
       else {
           {
               std::lock_guard<std::mutex> lock(_lock);
               pendingsend[fd]=std::move(fun);
           }
           _epoll->update_watch(fd, "wse");
       }
        return;
    }
    if (ev & EPOLLOUT ) {
        //DLOG(INFO) << "EPOLLOUT triggered.";
        auto f = pendingsend.find(fd);
        if (f == pendingsend.end())
        {
            LOG(ERROR) << "ready to write but no data to write.";
            close_socket(fd);
            return;
        }
        auto tosend = f->second(0);
        while (true) {
            
            if (tosend.first == nullptr)
            {
                {
                    std::lock_guard<std::mutex> lock(_lock);
                    pendingsend.erase(f);
                }
                close_socket(fd);
                break;

            }
            else
            {
                int cnt = write(fd, tosend.first, tosend.second);
                //DLOG_IF(INFO,cnt>0) << cnt << " bytes writtend";
                if (cnt == tosend.second)// all sended.
                {
                    
                   tosend= f->second(cnt);
                   continue;
                }
                else if (cnt < 0) {// failed to send any
                    if (errno != EAGAIN) {
                        DLOG(INFO) << "writed socket failed,errorno:"<<int(errno);
                        f->second(-1);
                        {
                            std::lock_guard<std::mutex> lock(_lock);
                            pendingsend.erase(f);
                        }
                        close_socket(fd);
                    }
                    else {
                     
                        break;
                    }

                }
                else {//partial sended,wait next trigger
                    DLOG(INFO) << "socket wirte buf is full,wait next trigger";
                    f->second(cnt);
                    break;
                }

            }
        }
        return;
    }
    if (ev & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
        auto f = pendingsend.find(fd);
        if (f != pendingsend.end())
        {
            f->second(-1);
            std::lock_guard<std::mutex> lock(_lock);
            pendingsend.erase(f); 
        }
        close_socket(fd);
        return;

    }
    DLOG(INFO) << "Un handled fd:"<< fd<<", event:"<<ev;
}

