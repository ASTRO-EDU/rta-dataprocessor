#include <iostream>

#include "WFSchema.hh"
#include "AvroEntities.hh"
#include "WFGenerator.hh"
#include "Utils.hh"

using namespace WF;

std::queue<std::vector<uint8_t>> serializedQueue;

int main() {

    auto generator = WFGenerator(1);
    
    auto ser = AvroSerializer<HeaderandWaveform>(serializedQueue);
    auto dser = AvroDeserializer<HeaderandWaveform>(serializedQueue);

    HeaderandWaveform genval = generator.get();
    print_data(genval, 10);
    ser.encode(&genval);
    std::cout << "MAIN:Lenght of queue: " << serializedQueue.size() << std::endl;
    
    
    size_t total_size = sizeof(genval) + genval.data.size() * sizeof(int32_t);
    std::cout << "Total memory used by packet (including vector data): " 
            << total_size << " bytes" << std::endl;

    // Get the size of the serialized packet
    size_t serialized_size = serializedQueue.front().size();
    std::cout << "Message size (serialized): " << serialized_size << " bytes" << std::endl;

    // Calculate the compression ratio
    double compression_ratio = static_cast<double>(total_size) / static_cast<double>(serialized_size);
    std::cout << "Compression ratio: " << compression_ratio << std::endl;


    HeaderandWaveform retval= dser.decode();

    if ( areEqual(retval, genval) ){
        std::cout << "Serialization successful" << std::endl;
    }
    else{
        std::cout << "Serialization failed" << std::endl;
    }

}