#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>
#include <future>
#include <atomic>
#include <vector>
#include <queue>

/**
	用C++11 新特性实现的线程池， 任务类型： function

	任务队列: no priority queue
	Author: Luo Yangxing
	Email: 673592308@qq.com
**/

//命名空间
namespace myThreadpool {
	class threadpool;

	enum PRIORITY {
		MIN = 1, NORMAL = 25, MAX = 50
	};

	class Task;
}

class myThreadpool::threadpool {
	using Task = std::function<void()>;
private:
	//线程池
	std::vector<std::thread> pool_threads;
	//任务队列
	std::queue<Task> tasks;
	//同步锁
	std::mutex pool_mutex;
	std::condition_variable pool_condition;
	//是否关闭提交
	std::atomic<bool> stop;
	std::atomic<bool> exitpool;
public:
	threadpool(size_t size = 4):stop{false},exitpool{false} {
		size = size < 1 ? 1:size;
		for(size_t i = 0;i < size; ++i) {
			pool_threads.emplace_back( //&TaskExecutor::schedual, this);
				[this] {
					for(;;) {
						Task task;

						{
							std::unique_lock<std::mutex> lock {pool_mutex};

							pool_condition.wait(lock, [this]{ return this->exitpool.load() || !this->tasks.empty();}); //阻塞直到任务队列不为空
							if(this->exitpool && tasks.empty())
							{
								std::cout<<"thread exit"<<std::endl;
								return;
							}

							task = std::move(this->tasks.front()); //从任务队列中取一个任务，放到一个线程中执行
							tasks.pop(); //从任务队列中删除该任务
						}

						task();//执行任务
					}
				});
		} 
	}
	~threadpool() {

		{
			std::unique_lock<std::mutex> lock {pool_mutex};
			this->stop.store(true);
		}
		pool_condition.notify_all();
		for(std::thread& thread : pool_threads) {
			//thread.detach(); //让线程自己结束，主线不用等代
			if(thread.joinable())
				thread.join(); //主线程等待子线程的结束
		}
	}

	//停止提交线程任务
	void shutdown() {
		this->stop.store(true);
	}

	void exit() {
		this->exitpool.store(true);
		this->stop.store(true);
		pool_condition.notify_all();

		for(std::thread& thread : pool_threads) {
			//thread.detach(); //让线程自己结束，主线不用等代
			thread.join(); //主线程等待子线程的结束
		}
	}
	//重启任务提交
	void restart() {
		this->stop.store(false);
	}
	//commit a task
	template <typename F, typename... Args>
	auto commitTask(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
		if(stop.load()) {
			throw std::runtime_error("Task executor has closed commit");
		}
		//using ResType = decltype(f(args...));
		typedef decltype(f(args...)) ResType; 
		//typename std::result_of<F(Args...)>::type 函数f的返回值类型
		auto task = std::make_shared<std::packaged_task<ResType()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		{
			std::lock_guard<std::mutex> lock {pool_mutex};
			tasks.emplace([task]() {
				(*task)();
			}); //lambda
			//std::cout<<"This task has put into the thread pool"<<std::endl;
		}
		pool_condition.notify_one(); //wakeup the threads
		//std::cout<<"wakeup one thread!"<<std::endl;

		std::future<ResType> future = task->get_future();
		return future;
	}

private:
	//获取一个待执行的task
	Task get_one_task() {
		std::unique_lock<std::mutex> lock {pool_mutex};
		pool_condition.wait(lock, [this]() {return !tasks.empty();}); //wait until has a task in execute
		Task task {std::move(tasks.front())}; //get a task
		tasks.pop();
		return task;
	}

	//任务调度
	void schedual() {
		while(true) {
			if(Task task = get_one_task()) {
				task();
			}
			else
			{
				throw std::runtime_error("Get a task failded!");//return;
			}
		}
	}
};


class myThreadpool::Task {
public:
	Task() {}
	void SetPriority(int priority) {
		if(priority > (PRIORITY::MAX))
			priority_ = (PRIORITY::MAX);
		else if(priority < (PRIORITY::MIN))
			priority_ = (PRIORITY::MIN);
		else
			priority_ = priority;
	}
	virtual void run() = 0;
protected:
	int priority_;
};

#endif