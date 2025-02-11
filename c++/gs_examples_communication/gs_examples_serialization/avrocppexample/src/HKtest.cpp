#include <iostream>

#include "HKSchema.hh"
#include "AvroEntities.hh"
#include "HKGenerator.hh"
#include "Utils.hh"

using namespace HK;

std::queue<std::vector<uint8_t>> serializedQueue;

int main() {

    HKGenerator generator = HKGenerator(1);
    
    auto ser = AvroSerializer<HeaderHK>(serializedQueue);
    auto dser = AvroDeserializer<HeaderHK>(serializedQueue);

    HeaderHK genval = generator.get();
    print_data(genval);
    ser.encode(&genval);
    std::cout << "MAIN:Lenght of queue: " << serializedQueue.size() << std::endl;

    size_t total_size = sizeof(HeaderHK);
    std::cout << "Total memory used by packet: " 
            << total_size << " bytes" << std::endl;

    // Get the size of the serialized packet
    size_t serialized_size = serializedQueue.front().size();
    std::cout << "Message size (serialized): " << serialized_size << " bytes" << std::endl;

    // Calculate the compression ratio
    double compression_ratio = static_cast<double>(total_size) / static_cast<double>(serialized_size);
    std::cout << "Compression ratio: " << compression_ratio << std::endl;

    HeaderHK retval= dser.decode();

    

    if ( areEqual(retval, genval) ){
        std::cout << "Serialization successful" << std::endl;
    }
    else{
        std::cout << "Serialization failed" << std::endl;
    }

}