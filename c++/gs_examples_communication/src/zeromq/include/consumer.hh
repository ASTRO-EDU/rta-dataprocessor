#ifndef CONSUMER_HH
#define CONSUMER_HH

#include <zmq.hpp>
#include <iostream>

#include "utils.hh"

template <typename T>
class Consumer {
public:
    Consumer(zmq::context_t& context, const std::string& address)
        : socket(context, ZMQ_PULL) {
        socket.connect(address);
    }

    // Destructor to clean up the socket
    virtual ~Consumer() {
        try {
            socket.close();
        } catch (const zmq::error_t& e) {
            std::cerr << "Error closing socket: " << e.what() << std::endl;
        }
    }

    bool consume(T &value)
    {
        return receiveMessage(socket, [&value](const zmq::message_t &message) {
            // std::cout << "msg size is: " << message.size() << std::endl;
            memcpy(&value, message.data(), sizeof(T));
        });
    }

private:
    zmq::socket_t socket;
};

// Specialization for std::vector<T>
template <typename T>
class Consumer<std::vector<T>> {
public:
    Consumer(zmq::context_t& context, const std::string& address)
        : socket(context, ZMQ_PULL) {
        socket.connect(address);
    }

    // Destructor to clean up the socket
    virtual ~Consumer() {
        try {
            socket.close();
        } catch (const zmq::error_t& e) {
            std::cerr << "Error closing socket: " << e.what() << std::endl;
        }
    }

    bool consume(std::vector<T> &vec)
    {
        return receiveMessage(socket, [&vec](const zmq::message_t &message) {
            int32_t size;
            memcpy(&size, message.data(), sizeof(int32_t));
            // std::cout << "msg size is: " << message.size() << std::endl;
            vec.resize(size);
            memcpy(vec.data(), static_cast<const char *>(message.data()) + sizeof(int32_t), size * sizeof(T));
        });
    }


private:
    zmq::socket_t socket;
};

// Specialization for std::vector<T>
template <>
class Consumer<std::string> {
public:
    Consumer(zmq::context_t& context, const std::string& address)
        : socket(context, ZMQ_PULL) {
        socket.connect(address);
    }

    // Destructor to clean up the socket
    virtual ~Consumer() {
        try {
            socket.close();
        } catch (const zmq::error_t& e) {
            std::cerr << "Error closing socket: " << e.what() << std::endl;
        }
    }

    bool consume(std::string& str) {
        
        return receiveMessage(socket, [&str](const zmq::message_t &message) {
            size_t size ;
            memcpy( &size, message.data(), sizeof(size_t));
            str.resize(size);
            memcpy( str.data(), static_cast<const char *>(message.data()) + sizeof(size_t),  size );
        });

    }

private:
    zmq::socket_t socket;
};


#endif // CONSUMER_HH
