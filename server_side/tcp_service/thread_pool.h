#pragma once
#include<string>
#include<stdint.h>
#include<future>
#include<list>
#include<functional>
#include <atomic>
#include<mutex>
#include<queue>

class ThreadPool {
    struct ThreadData {
        using TaskType = std::function<void()>;
        std::queue<TaskType> task_queue;
        std::vector<std::thread> pool;
        std::condition_variable task_ready;
        std::atomic_bool working = true;
        int thread_size;
        std::mutex lock;
    };
	ThreadData* _data;
	void thread_loop(int id);
public:
	template <class F, class...Arg>
	auto pushTask(F f, Arg... args)->std::future<decltype(f(args...))>;
    template <class F, class...Arg>
    auto pushVoidTask(F f, Arg... args);
	ThreadPool(int threadnum = 5);
	~ThreadPool();
};

template<class F, class ...Arg>
inline auto ThreadPool::pushTask(F f, Arg  ...args)-> std::future<decltype(f(args...))>
{
    using ReturnType = decltype(f(args...));

    auto task = std::packaged_task<ReturnType()>(
        std::bind(std::forward<F>(f), std::forward<Arg>(args)...));

    std::future<ReturnType> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock(_data->lock);
        _data->task_queue.emplace(task);
    }

    _data->task_ready.notify_one();
    return res;
}


template<class F, class ...Arg>
inline auto ThreadPool::pushVoidTask(F f, Arg  ...args)
{
    auto task = std::bind(f, args...);

    {
        std::unique_lock<std::mutex> lock(_data->lock);
        _data->task_queue.emplace(task);
    }

    _data->task_ready.notify_one();

}
