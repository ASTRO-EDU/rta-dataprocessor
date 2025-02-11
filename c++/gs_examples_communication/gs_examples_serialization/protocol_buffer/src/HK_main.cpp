#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <google/protobuf/message.h>
 
#include "WFGenerator.hh"
#include "HKGenerator.hh"
#include "TimeSpecGen.hh"
#include "Utils.hh"
#include "ProtocolBufferEntities.hh"

#include "HK.pb.h"  // Include your generated header file for protobuf messages
#include "WF.pb.h"


int main() {
    std::queue<std::string> sharedQueue;
    MessageWriter<HeaderHK> writer(sharedQueue);
    MessageReader<HeaderHK> reader(sharedQueue);

    auto message_gen =  HKGenerator(1);
    // auto message_gen2 =  HKGenerator(1);
    
    // Generate a message and write it to the queue
    int N = 50;
    std::vector<HeaderHK> message(N) ;
    for (int i =0 ; i < N; i++){
        message[i] = message_gen.get();
        writer.writeMessage(message[i]);
    }
    size_t total_size = sizeof(message[0]);
    std::cout << "Approximate memory used by packet: " 
            << total_size << " bytes" << std::endl;

    // Get the size of the serialized packet
    size_t serialized_size = sharedQueue.front().size();
    std::cout << "Message size (serialized): " << serialized_size << " bytes" << std::endl;

    // Calculate the compression ratio
    double compression_ratio = static_cast<double>(total_size) / static_cast<double>(serialized_size);
    std::cout << "Compression ratio: " << compression_ratio << std::endl;

    // // Read the message from the queue
    std::vector<HeaderHK> recv_message(N) ;
    for (int i =0 ; i < N; i++){
        recv_message.at(i)=reader.readMessage() ;
        std::cout << "Received HeaderHK message:" << std::endl;
        std::cout << "APID: " << recv_message[i].apid() << std::endl;
        std::cout << "Counter: " << recv_message[i].counter() << std::endl;
        // Output other fields as needed
    } 
    std::cout << "Print first packet" << std::endl;
    print_HK(recv_message[0]);
    
    return 0;
}