#include <random>
#include <fstream>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <queue>

#include "WFGenerator.hh"
#include "ProtocolBufferEntities.hh"


#include "producer.hh"
#include "utils.hh"

int main(int argc, char* argv[]) {

    const int N_mes = parseArguments(argc, argv);
    const int N_mes_update = getMessageUpdate(N_mes);
    std::cout << "message update every " << N_mes_update << " msgs" << std::endl;

    std::string ip_port = getIpPortFromConfig("config.txt");

    std::cout << "Start Generating " << std::endl;

    std::queue<std::string> serializedQueue;
    std::signal(SIGINT, signalHandler);


    auto generator = WFGenerator(1);    
    MessageWriter<HeaderandWaveform> writer(serializedQueue);
    std::vector<HeaderandWaveform>* packets = new std::vector<HeaderandWaveform>(N_mes);

    auto t1 = std::chrono::system_clock::now();

    std::cout << "Start Generating " << std::endl;

    for (int i = 0; i < N_mes && !stop; ++i) {
        packets->at(i) = generator.get();
        // std::cout << "Packet size: " << packets[i].size() << std::endl;
        
        // std::cout <<  "Sending packet with run id: "<< genval.runID << ", decimation: " << genval.decimation << " and pc: " << genval.counter << std::endl;
       
        printLoopStatistics(i, N_mes_update, [&i](){
                std::cout << "Generated " << i << "msgs" << std::endl;
            });

    }

    auto t2 = std::chrono::system_clock::now();

    std::cout << "Start Serializing " << std::endl;

    for (const auto& value : *packets) {

        writer.writeMessage(value);
        
        // std::cout <<  "Sending packet with run id: "<< genval.runID << ", decimation: " << genval.decimation << " and pc: " << genval.counter << std::endl;
       
        printLoopStatistics(serializedQueue.size(), N_mes_update, [&serializedQueue](){
            std::cout << "Serialized " << serializedQueue.size() << "msgs" << std::endl;
        });

    }

    size_t total_size = sizeof(packets[0]) + packets->at(0).data().size() * sizeof(packets->at(0).data().Get(0));

    size_t serialized_size = packets->at(0).ByteSizeLong();
    delete packets;

    zmq::context_t context(1);
    Producer<std::string>* producer = new Producer<std::string>(context, "tcp://" + ip_port);

    auto t3a = std::chrono::system_clock::now();

    std::cout << "Start Sending" << std::endl;
    bool first = true;
    std::chrono::_V2::system_clock::time_point t3b;
    while (!stop)
    {
        if(first){
            first = false;
            t3b = std::chrono::system_clock::now();
        }
        
        producer->produce(serializedQueue.front());
        serializedQueue.pop();
        
        printLoopStatistics(serializedQueue.size(), N_mes_update, [&serializedQueue](){
            std::cout << "Remaining packets to send: " << serializedQueue.size() << std::endl;
        });

        if (serializedQueue.size()<=0){
            break;
        }
    }
    
    
    std::cout << "Done" << std::endl;

    auto t4 = std::chrono::system_clock::now();
    
    std::chrono::duration<double> generation_seconds = t2 - t1;
    std::chrono::duration<double> serialization_seconds = t3a - t2;
    std::chrono::duration<double> comm_seconds = t4 - t3b;
    

    // Display the time difference in seconds
    std::cout << "Produced number of messages: " << N_mes << std::endl ;
    std::cout << "Packet generation time : " << generation_seconds.count() << " seconds";
    std::cout << "; " << N_mes/generation_seconds.count() << " packet/s\n";
    std::cout << "Serialization time : " << serialization_seconds.count() << " seconds";
    std::cout << "; " << N_mes/serialization_seconds.count() << " packet/s\n";
    std::cout << "Communication time : " << comm_seconds.count() << " seconds";
    std::cout << "; " << N_mes/comm_seconds.count() << " packet/s\n";
    std::cout << "Total Sending time : " << serialization_seconds.count() + comm_seconds.count() << " seconds";
    std::cout << "; " << N_mes/(serialization_seconds.count() + comm_seconds.count()) << " packet/s\n";

    std::cout << "Approximate memory used by packet: " 
            << total_size << " bytes" << std::endl;

            // Get the size of the serialized packet
    std::cout << "Calculated Message size (serialized): " << serialized_size << " bytes" << std::endl;

    // Calculate the compression ratio
    double compression_ratio = static_cast<double>(total_size) / static_cast<double>(serialized_size);
    std::cout << "Compression ratio: " << compression_ratio << std::endl;
    delete producer;
    context.close();

    return 0;
 
}
