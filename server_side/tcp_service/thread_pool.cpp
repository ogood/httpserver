#include "thread_pool.h"
#include<common/utility.h>


ThreadPool::ThreadPool(int num)
{
    _data = new ThreadData;
    _data->thread_size = num;
    _data->pool.resize(num);
	for (int i = 0; i < num; i++) {
        _data->pool[i] =std::thread( &ThreadPool::thread_loop,this,i);

	}
}

ThreadPool::~ThreadPool()
{
    _data->working.store(false);
    _data->task_ready.notify_all();
    for (auto& thread : _data->pool) {
        thread.join();
    }
    delete _data;
}

void ThreadPool::thread_loop(int id)
{
    std::unique_lock<std::mutex> lock(_data->lock);

    while (true) {
        if (_data->task_queue.size()) {
            auto task = std::move(_data->task_queue.front());
            _data->task_queue.pop();//return void
            lock.unlock();
            DLOG(INFO) << "thread pool working..id:"<<id;
            task();
            lock.lock();
        }
        else if (_data->working.load()==false) {
            break;
        }
        else {
            _data->task_ready.wait(lock);
        }
    }

}