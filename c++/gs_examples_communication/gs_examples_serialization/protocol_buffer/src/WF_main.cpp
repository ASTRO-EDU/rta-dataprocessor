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
    MessageWriter<HeaderandWaveform> writer(sharedQueue);
    MessageReader<HeaderandWaveform> reader(sharedQueue);


    auto message_gen =  WFGenerator(1);
    // auto message_gen2 =  HKGenerator(1);
    
    // Generate a message and write it to the queue
    // HeaderHK message2 = message_gen2.get();
    // print_WF(message);
    // print_HK(message2);
    // Generate a message and write it to the queue
    int N = 50;
    std::vector<HeaderandWaveform> message(N);
    for (int i =0 ; i < N; i++){
        message[i] = message_gen.get();
        writer.writeMessage(message[i]);
    }

    size_t total_size = sizeof(message[0]) + message[0].data().size() * sizeof(message[0].data().Get(0));
    std::cout << "Approximate memory used by packet: " 
            << total_size << " bytes" << std::endl;

            // Get the size of the serialized packet
    size_t serialized_size = message[0].ByteSizeLong();
    std::cout << "Calculated Message size (serialized): " << serialized_size << " bytes" << std::endl;

    // Calculate the compression ratio
    double compression_ratio = static_cast<double>(total_size) / static_cast<double>(serialized_size);
    std::cout << "Compression ratio: " << compression_ratio << std::endl;


    // // Read the message from the queue
    std::vector<HeaderandWaveform> recv_message(N) ;
    for (int i =0 ; i < N; i++){
        recv_message.at(i)=reader.readMessage() ;
        // std::cout << "Received HeaderandWaveform message:" << std::endl;
        // std::cout << "APID: " << recv_message[i].apid() << std::endl;
        // std::cout << "Counter: " << recv_message[i].counter() << std::endl;
        // std::cout << "Lenght of data: " << recv_message[i].data_size() << std::endl;
        // Output other fields as needed
    }
    std::cout << "Print first packet" << std::endl;
    print_WF(recv_message[0], 10);
    return 0;
}