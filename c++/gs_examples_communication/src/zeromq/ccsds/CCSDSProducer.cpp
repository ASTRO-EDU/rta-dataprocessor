#include <random>
#include <fstream>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <queue>

#include "generators.hh"
#include "tchandler.h"

#include "producer.hh"
#include "utils.hh"
#include "utils2.hh"


int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    
    const int N_mes = parseArguments(argc, argv);
    const int N_mes_update = getMessageUpdate(N_mes);
    std::cout << "message update every " << N_mes_update << " msgs" << std::endl;

    std::string ip_port = getIpPortFromConfig("config.txt");
    std::cout << "Sending with socket: " << ip_port << std::endl;

    std::queue<std::vector<uint8_t>> serializedQueue;
    std::signal(SIGINT, signalHandler);

    auto hkgenerator = HKGenerator(1);
    auto wfgenerator = WFGenerator(1);
    auto t1 = std::chrono::system_clock::now();

    std::cout << "Start Generating " << std::endl;

    int packet_count = 0;
    for (int i = 0; i < N_mes && !stop; ++i) {
        if (i % 2 == 0) {  // Alternate between WF and HK packets
            HeaderWF wfPacket = wfgenerator.get();
            pushPacketToQueue(serializedQueue, wfPacket);
        }
        else {
            HeaderHK hkPacket = hkgenerator.get();
            pushPacketToQueue(serializedQueue, hkPacket);
        }
        packet_count++;
    }

    zmq::context_t context(1);
    Producer<std::vector<uint8_t>>* producer = new Producer<std::vector<uint8_t>>(context, "tcp://" + ip_port);

    auto t2a = std::chrono::system_clock::now();

    std::cout << "Start Sending" << std::endl;
    
    bool first = true;
    std::chrono::_V2::system_clock::time_point t2b;

    auto saved = serializedQueue.back();

    while (!stop){
        if(first){
            first = false;
            t2b = std::chrono::system_clock::now();
        }

        // Send the generated message via ZeroMQ to the consumer
        producer->produce(serializedQueue.front());
        serializedQueue.pop();
        
        printLoopStatistics(serializedQueue.size(), N_mes_update, [&serializedQueue](){
            std::cout << "Remaining packets to send: " << serializedQueue.size() << std::endl;
        });

        if (serializedQueue.empty()){     
            break;
        }
    }

    std::cout << "Done" << std::endl;

    auto t3 = std::chrono::system_clock::now();
    
    std::chrono::duration<double> generation_seconds = t2a - t1;
    std::chrono::duration<double> comm_seconds = t3 - t2b;
    
    std::cout << "Printing last packet" << std::endl;
    if (packet_count % 2 != 0) {
        HeaderWF::print(*reinterpret_cast<const HeaderWF*>(saved.data()), 10);
    }
    else {
        HeaderHK::print(*reinterpret_cast<const HeaderHK*>(saved.data()));
    }
    
    // Display the time difference in seconds
    std::cout << "Produced number of messages: " << N_mes << std::endl ;
    std::cout << "Packet generation time : " << generation_seconds.count() << " seconds";
    std::cout << "; " << N_mes/generation_seconds.count() << " packet/s\n";
    std::cout << "Sending time : " << comm_seconds.count() << " seconds";
    std::cout << "; " << N_mes/comm_seconds.count() << " packet/s\n";
    size_t total_size = sizeof(HeaderWF);
    std::cout << "Total memory used by packet: " 
            << total_size << " bytes" << std::endl;
    delete producer;
    context.close();

    return 0;
}
