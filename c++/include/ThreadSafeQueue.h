#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable condvar;

    bool _stop = false; // Flag for stopping

public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(value);
        condvar.notify_one();
    }

    T front() {
        std::unique_lock<std::mutex> lock(mtx);
        condvar.wait(lock, [this] { return _stop || !queue.empty(); });

        if (_stop) {  // if (_stop && queue.empty()) 
            throw std::runtime_error("ThreadSafeQueue stopped");
        }

        return queue.front();
    }

    T get() {
        std::unique_lock<std::mutex> lock(mtx);
        condvar.wait(lock, [this] { return _stop || !queue.empty(); });

        if (_stop) {  // if (_stop && queue.empty()) 
            throw std::runtime_error("ThreadSafeQueue stopped");
        }

        T value = queue.front();
        queue.pop();
        return value;
    }

    void pop() {
        std::unique_lock<std::mutex> lock(mtx);
        condvar.wait(lock, [this] { return _stop || !queue.empty(); });

        if (_stop) {  // if (_stop && queue.empty()) 
            return; // Stops and exits
        }

        queue.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

    void notify_all() {
        std::lock_guard<std::mutex> lock(mtx);
        _stop = true;
        condvar.notify_all();
    }

};

#endif // THREADSAFEQUEUE_H