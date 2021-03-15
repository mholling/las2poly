#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class Queue {
	bool closed;
	std::queue<T> queue;
	std::mutex mutex;
	std::condition_variable consumers;

public:
	Queue() : closed(false) { }

	auto operator>>(T& element) {
		for (std::unique_lock lock(mutex); !(closed && queue.empty()); consumers.wait(lock))
			if (!queue.empty()) {
				element = queue.front();
				queue.pop();
				return true;
			}
		return false;
	}

	auto &operator<<(const T& element) {
		std::unique_lock lock(mutex);
		// TODO: do something if the queue is closed
		queue.push(element);
		lock.unlock();
		consumers.notify_one();
		return *this;
	}

	auto close() {
		std::unique_lock lock(mutex);
		closed = true;
		lock.unlock();
		consumers.notify_all();
	}
};
 
#endif
