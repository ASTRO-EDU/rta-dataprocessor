#ifndef AVRO_ENTITIES_HH
#define AVRO_ENTITIES_HH

#include <fstream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include "avro/Compiler.hh"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"
#include "avro/Generic.hh"
#include <iostream>
#include <cstdlib> // For rand()
#include <ctime>   // For clock_gettime()
#include "HKSchema.hh"
#include "WFSchema.hh"
#include "WFGenerator.hh"
#include "Utils.hh"

const size_t chunk_hk = sizeof(HK::HeaderHK)/10;
const size_t chunk_wf = (sizeof(WF::HeaderandWaveform) + sizeof(uint32_t) * WF_SIZE)/15;

// Forward declaration of print_data
template <typename T>
void print_data(const T&);

// AvroSerializer class definition
template <class T>
class AvroSerializer {
private:
    std::queue<std::vector<uint8_t>>& _queue;

public:
    // Constructor
    AvroSerializer(std::queue<std::vector<uint8_t>>& q);

    // Method to encode data of type T
    void encode(const T* data);
};

// AvroSerializer class implementation
template <class T>
AvroSerializer<T>::AvroSerializer(std::queue<std::vector<uint8_t>>& q)
    : _queue(q) {}

template <class T>
void AvroSerializer<T>::encode(const T* data) {
    
    std::shared_ptr<avro::OutputStream> out = avro::memoryOutputStream();
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init(*out);
    avro::encode(*e, *data);

    std::shared_ptr<std::vector<uint8_t>> p = avro::snapshot(*out);

    // std::cout << "Vector values are: ";
    // for (auto byte : *p) {
    //     std::cout << static_cast<int>(byte) << " ";
    // }
    // std::cout << std::endl;

    _queue.push(*p);
    // std::cout << "Length of queue: " << serializedQueue.size() << std::endl;
}

template <>
void AvroSerializer<HK::HeaderHK>::encode(const HK::HeaderHK* data) {
    
    std::shared_ptr<avro::OutputStream> out = avro::memoryOutputStream(chunk_hk);
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init(*out);
    avro::encode(*e, *data);

    std::shared_ptr<std::vector<uint8_t>> p = avro::snapshot(*out);

    // std::cout << "Vector values are: ";
    // for (auto byte : *p) {
    //     std::cout << static_cast<int>(byte) << " ";
    // }
    // std::cout << std::endl;

    _queue.push(*p);
    // std::cout << "Length of queue: " << serializedQueue.size() << std::endl;
}

template <>
void AvroSerializer<WF::HeaderandWaveform>::encode(const WF::HeaderandWaveform* data) {
    
    std::shared_ptr<avro::OutputStream> out = avro::memoryOutputStream(chunk_wf);
    avro::EncoderPtr e = avro::binaryEncoder();
    e->init(*out);
    avro::encode(*e, *data);

    std::shared_ptr<std::vector<uint8_t>> p = avro::snapshot(*out);

    // std::cout << "Vector values are: ";
    // for (auto byte : *p) {
    //     std::cout << static_cast<int>(byte) << " ";
    // }
    // std::cout << std::endl;

    _queue.push(*p);
    // std::cout << "Length of queue: " << serializedQueue.size() << std::endl;
}

// AvroDeserializer class definition
template <class T>
class AvroDeserializer {
private:
    std::queue<std::vector<uint8_t>>& _queue;
    T c;
    bool stop;

public:
    // Constructor
    AvroDeserializer(std::queue<std::vector<uint8_t>>& q);

    // Method to decode data of type T
    T decode();
};

// AvroDeserializer class implementation
template <class T>
AvroDeserializer<T>::AvroDeserializer(std::queue<std::vector<uint8_t>>& q)
    : _queue(q) {}

template <class T>
T AvroDeserializer<T>::decode() {
    try {
        // std::cout << "Length of queue: " << serializedQueue.size() << std::endl;

        if (!_queue.empty()) {
            std::vector<uint8_t> dequeuedData = _queue.front();
            _queue.pop();

            std::unique_ptr<avro::InputStream> in = avro::memoryInputStream(dequeuedData.data(), dequeuedData.size());
            avro::DecoderPtr d = avro::binaryDecoder();
            d->init(*in);
            avro::decode(*d, c);

            // Assuming print is a function to print T data
            // print_data(c2);  // Replace or adjust according to your needs

            return c;
        } else {
            std::cout << "Length of queue: " << _queue.size() << std::endl;
            throw std::runtime_error("No more data to deserialize");
        }
    } catch (const std::exception& e) {
        std::cerr << "Deserialization error: " << e.what() << std::endl;
        throw;
    }
}

template <>
void print_data(const WF::HeaderandWaveform& data) {
    print_WF(data);
}
void print_data(const WF::HeaderandWaveform& data, const int limit) {
    print_WF(data, limit);
}

template <>
void print_data(const HK::HeaderHK& data) {
    print_HK(data);
}

template <typename T>
void print_data(const T& data) {
    std::cerr << "No valid type to print" << std::endl;
}



#endif // AVRO_ENTITIES_HH
