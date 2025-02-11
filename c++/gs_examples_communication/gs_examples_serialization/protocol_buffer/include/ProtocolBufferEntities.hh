#ifndef PB_ENTITIES_HH
#define PB_ENTITIES_HH

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <google/protobuf/message.h>
#include "HK.pb.h"  // Include your generated header file for protobuf messages
#include "Utils.hh"

// Writer class
template <class T> class MessageWriter {
public:
    MessageWriter(std::queue<std::string>& queue) : _queue(queue) {}

    void writeMessage(const T& message) {
        // Serialize the message to a string
        std::string serialized_message ;
        if (!message.SerializeToString(&serialized_message)) {
            std::cerr << "Failed to serialize the message." << std::endl;
            return;
        }
        // std::cout << "Serialized size:" << serialized_message.size() << std::endl;

        // Push the serialized message into the shared queue
        _queue.push(serialized_message);

    }

private:
    std::queue<std::string>& _queue;
};

// Reader class
template <class T> class MessageReader {
public:
    MessageReader(std::queue<std::string>& queue) : _queue(queue) {}

    T readMessage() {
    //     std::string serialized_message;
    //     if (!_queue.pop(serialized_message)) {
    //         std::cerr << "Failed to pop message from queue." << std::endl;
    //         return false;
    //     }

    //     // Deserialize the message from the string
    //     if (!message.ParseFromString(serialized_message)) {
    //         std::cerr << "Failed to deserialize the message." << std::endl;
    //         return false;
    //     }

    //     return true;
    // }
    try {
        // std::cout << "Length of queue: " << serializedQueue.size() << std::endl;
        T message;
        if (!_queue.empty()) {
            std::string serialized_message = _queue.front();
            _queue.pop();

            if (!message.ParseFromString(serialized_message)) {
                std::runtime_error("Failed to deserialize the message.");
            }

            return message;

        } else {
            std::cout << "Length of queue: " << _queue.size() << std::endl;
            throw std::runtime_error("No more data to deserialize");
        }
    } catch (const std::exception& e) {
        std::cerr << "Deserialization error: " << e.what() << std::endl;
        throw;
    }
    }
private:
    std::queue<std::string>& _queue;
};

#endif