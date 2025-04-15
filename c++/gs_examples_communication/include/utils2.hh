#ifndef GS_COMM_UTILS_HH2
#define GS_COMM_UTILS_HH2
#include <atomic>
#include <string>
#include <zmq.hpp>
#include <functional>
#include <random>
#include <fstream>
#include <csignal>
#include <condition_variable>
#include <queue>

#include "generators.hh"
#include "tchandler.h"

#include "../src/zeromq/include/producer.hh"

// Function to serialize packets of type T (HeaderWF or HeaderHK) into a vector which is then pushed into the queue
template <typename T>
void pushPacketToQueue(std::queue<std::vector<uint8_t>>& queue, const T& packet) {
    std::vector<uint8_t> serializedPacket;
    size_t packetSize = sizeof(T);

    // Resize the packet to hold packet data
    serializedPacket.resize(packetSize);

    // Copy the actual packet data
    memcpy(serializedPacket.data(), &packet, packetSize);

    queue.push(std::move(serializedPacket));    // Move (rather than copying) to avoid unnecessary memory allocations
}

// Function to serialize packets of type T into a vector and return them
template <typename T>
std::vector<uint8_t> serializePacket(const T& packet) {
    int32_t size = sizeof(T);
    std::vector<uint8_t> buffer(sizeof(int32_t) + size);  // Allocate extra space for size
    memcpy(buffer.data(), &size, sizeof(int32_t));        // Copy size at the start
    memcpy(buffer.data() + sizeof(int32_t), &packet, size); // Copy packet data

    return buffer;
}
#endif