#ifndef __TCHANDLER_H__
#define __TCHANDLER_H__

#include <cstdint> 
#include <queue>
#include <vector>
#include <cstring>  // For memcpy
#include <iostream> // For std::cerr

template <class DataType> 
class TcHandler {
public:
    // Constructor
    TcHandler(std::queue<std::vector<uint8_t>>& q);

    // Destructor
    ~TcHandler();

    // Bufferize (serialize) the DataType object
    void bufferize(const DataType& packet);

    // Debufferize (deserialize) the DataType object
    bool debufferize(DataType& packet);

private:
    std::queue<std::vector<uint8_t>>& serializedQueue;
};

template <class DataType>
TcHandler<DataType>::TcHandler(std::queue<std::vector<uint8_t>>& q)
    : serializedQueue(q) {}

template <class DataType>
TcHandler<DataType>::~TcHandler() {}

template <class DataType>
void TcHandler<DataType>::bufferize(const DataType& packet) {
    // Create a buffer with the size of the DataType
    std::vector<uint8_t> buff(sizeof(DataType));
    
    // Copy DataType object to buffer
    std::memcpy(buff.data(), &packet, sizeof(DataType));

    // Push buffer into the queue
    serializedQueue.push(std::move(buff));
}

template <class DataType>
bool TcHandler<DataType>::debufferize(DataType& packet) {
    // Check if the queue is empty
    if (serializedQueue.empty()) {
        std::cerr << "Queue is empty, cannot debufferize." << std::endl;
        return false;
    }

    // Get the front of the queue
    std::vector<uint8_t> buff = std::move(serializedQueue.front());
    serializedQueue.pop();

    // Check if buffer size is correct
    if (buff.size() != sizeof(DataType)) {
        std::cerr << "Buffer size mismatch." << std::endl;
        return false;
    }

    // Copy buffer to DataType object
    std::memcpy(&packet, buff.data(), sizeof(DataType));

    return true;
}

#endif // __TCHANDLER_H__
