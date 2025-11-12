#ifndef THREADSAFEQUEUE_H
#define THREADSAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

// Class used to redefine the standard C++ queue methods, in order to make them more thread-safe and concurrent-access safe
template <typename T>   // To work with any data type
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    mutable std::mutex mtx;     // Mutex for thread synchronization
    std::condition_variable condvar;    // For blocking operations
    bool _stop = false; // Flag for stopping

public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    // Thread-safe push
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(value);
        condvar.notify_one();
    }

    // Thread-safe front
    T front() {
        std::unique_lock<std::mutex> lock(mtx);
        // If the wait predicate returns true (queue has data or stop requested), continue execution. 
        // If the predicate returns false (queue empty and not stopping), release the lock, put the thread to sleep and wait for a notification.
        // This prevents spurious wake-ups, lost notifications and race conditions
        condvar.wait(lock, [this] { return _stop || !queue.empty(); });     

        if (_stop) {  // if (_stop && queue.empty()) 
            throw std::runtime_error("ThreadSafeQueue stopped");
        }

        return queue.front();
    }

    // Combination of atomic front + pop
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

    // Thread-safe pop
    void pop() {
        std::unique_lock<std::mutex> lock(mtx);
        condvar.wait(lock, [this] { return _stop || !queue.empty(); });

        if (_stop) {  // if (_stop && queue.empty()) 
            return; // Stops and exits
        }

        queue.pop();
    }

    // Thread-safe empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    // Thread-safe size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }

    // Used to wake up all threads waiting on the queues in order to have a clean shutdown
    void notify_all() {
        std::lock_guard<std::mutex> lock(mtx);
        _stop = true;
        condvar.notify_all();
    }

};

#endif // THREADSAFEQUEUE_H