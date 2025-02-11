#include <iostream> // Include for input/output operations
#include <queue> // Include for using the queue data structure
#include <thread> // Include for using thread functionality
#include <mutex> // Include for using mutex for synchronization
#include <condition_variable> // Include for using condition variables
#include <csignal>
#include <atomic>
#include <random>


std::atomic<bool> stop{false};


std::mutex mtx; // Mutex for synchronization
std::condition_variable cond_var1; // Condition variable for producer-consumer signaling
std::condition_variable cond_var2; // Condition variable for producer-consumer signaling
int N_mes = 100000000;
    int N_cons = 0;
// Signal handler for SIGINT
void signalHandler(int signal) {
    if (signal == SIGINT) {
        stop = true;
        cond_var1.notify_all();
        cond_var2.notify_all();
        std::cout << "\nSIGINT received. Setting stop flag." << std::endl;
    }
}

const unsigned int MAX_BUFFER_SIZE = 10; // Maximum size of the buffer

template <typename T> class Producer {
private:
    std::queue<T>& m_buffer;
    std::mutex& m_mtx; // Mutex for synchronization

    std::unique_lock<std::mutex> m_lock; // Lock the mutex
public:
    Producer( std::queue<T>& buffer, std::mutex& mtx ) : m_buffer(buffer) , m_mtx(mtx) , m_lock(m_mtx, std::defer_lock){} ;
    void produce(T value) {

        m_lock.lock(); // Lock the mutex
        cond_var2.wait(m_lock, [this] { return m_buffer.size() < MAX_BUFFER_SIZE || stop; }); // Wait until there's space in buffer
        if (!stop){
        m_buffer.push(value); // Add value to the buffer
        // std::cout << "Producing " << value << std::endl; // Output the produced value
        // std::cout << "Buffer size after producing: " << m_buffer.size() << std::endl << std::endl; // Display buffer size after producing

        m_lock.unlock(); // Unlock the mutex
        cond_var1.notify_one();// Notify one waiting thread
        }
    }
};

template <typename T> class Consumer
{
    private:
        std::queue<T>& m_buffer;
        std::mutex& m_mtx; // Mutex for synchronization
        std::unique_lock<std::mutex> m_lock; // Lock the mutex
    public:
        Consumer( std::queue<T>& buffer, std::mutex& mtx
        ) : m_buffer(buffer) , m_mtx(mtx) , m_lock(m_mtx, std::defer_lock) {} ;

        void consume() {

        m_lock.lock(); // Lock the mutex
        cond_var1.wait(m_lock, [this] { return m_buffer.size() > 0 || stop; }); // Wait until there's something in the buffer
        if (!stop){
        int value = m_buffer.front(); // Get the front value from buffer
        m_buffer.pop(); // Remove the value from buffer
        // std::cout << "Consuming " << value << std::endl; // Output the consumed value
        // std::cout << "Buffer size after consuming: " << m_buffer.size() << std::endl << std::endl; // Display buffer size after consuming

        m_lock.unlock(); // Unlock the mutex
        cond_var2.notify_one(); // Notify one waiting thread
        }
        }
};


int main() {

    std::signal(SIGINT, signalHandler);

    std::queue<int> buffer; // Queue to act as a buffer
    Producer<int> producer(buffer, mtx);
    Consumer<int> consumer1(buffer, mtx);
    Consumer<int> consumer2(buffer, mtx);

    
    std::mutex m_mtx2; // Mutex for synchronization

    std::unique_lock<std::mutex> m_lock{m_mtx2, std::defer_lock}; // Lock the mutex

    std::thread producerThread([&producer]() {
        std::random_device rd; // Obtain a random number from hardware
        std::mt19937 gen(rd()); // Seed the generator
        std::uniform_int_distribution<> distr(1, 100); // Define the range

        for (int i = 0; i < N_mes; ++i) {
            int randomValue = distr(gen);
            producer.produce(randomValue);
        }
    }
    );

    auto lambda = [] (Consumer<int>& consumer, std::unique_lock<std::mutex>& m_lock) {
        while (N_cons<N_mes && !stop) {
            
            consumer.consume(); // Consume a value
            // Create a thread to consume values
            m_lock.lock();
            N_cons++;
            if (N_cons % 1000000 == 0) {
                std::cout << "Consumed " << N_cons << " messages." << std::endl;
            }
            m_lock.unlock();
            
        }
    };

    std::thread consumerThread( lambda, std::ref(consumer1), std::ref(m_lock) );
    // std::thread consumerThread2( lambda, std::ref(consumer2) ); // Create another consumer thread

    producerThread.join(); // Wait for producer thread to finish
    consumerThread.join(); // Wait for consumer thread to finish

    return 0;
}