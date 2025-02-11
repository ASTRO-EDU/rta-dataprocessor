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

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    
    const int N_mes = parseArguments(argc, argv);
    const int N_mes_update = getMessageUpdate(N_mes);
    std::cout << "message update every " << N_mes_update << " msgs" << std::endl;

    std::string ip_port = getIpPortFromConfig("config.txt");
    std::cout << "Sending with socket: " << ip_port << std::endl;
    std::queue<HeaderWF> serializedQueue;
    std::signal(SIGINT, signalHandler);

    auto generator = WFGenerator(1);    
    auto t1 = std::chrono::system_clock::now();

    std::cout << "Start Generating " << std::endl;

    for (int i = 0; i < N_mes && !stop; ++i) {
        serializedQueue.push(generator.get());
        // std::cout <<  "Sending packet with run id: "<< genval.runID << ", decimation: " << genval.decimation << " and pc: " << genval.counter << std::endl;
       
        printLoopStatistics(i, N_mes_update, [&i](){
                std::cout << "Generated " << i << "msgs" << std::endl;
            });
    }
    
    zmq::context_t context(1);
    Producer<HeaderWF>* producer = new Producer<HeaderWF>(context, "tcp://" + ip_port);

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

    auto t3 = std::chrono::system_clock::now();
    
    std::chrono::duration<double> generation_seconds = t2a - t1;
    std::chrono::duration<double> comm_seconds = t3 - t2b;
    
    std::cout << "Printing last packet" << std::endl ;

    HeaderWF::print(saved, 10);
    
    // Display the time difference in seconds
    std::cout << "Produced number of messages: " << N_mes << std::endl ;
    std::cout << "Packer generation time : " << generation_seconds.count() << " seconds";
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
