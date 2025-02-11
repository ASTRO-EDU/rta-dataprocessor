#include <fstream>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <queue>

#include "generators.hh"
#include "tchandler.h"

#include "consumer.hh"
#include "utils.hh"

int main(int argc, char* argv[]) {

    std::signal(SIGINT, signalHandler);
    
    const int N_mes = parseArguments(argc, argv);
    const int N_mes_update = getMessageUpdate(N_mes);
    std::cout << "message update every " << N_mes_update << " msgs" << std::endl;

    std::string ip_port = getIpPortFromConfig("config.txt");

    std::queue<HeaderWF> serializedQueue;

    std::cout << "Start Receiving " << std::endl;

    zmq::context_t context(1);
    Consumer<HeaderWF>* consumer = new Consumer<HeaderWF>(context, "tcp://" + ip_port);
    
    int N_cons = 0;
    bool first = true;
    std::chrono::_V2::system_clock::time_point t1;
    while (!stop) {
        HeaderWF value;
        if (consumer->consume(value)) {
            if (first){
                first = false;
                t1 = std::chrono::system_clock::now();
            }
            serializedQueue.push(std::move(value));  // Push the consumed value into the queue
            ++N_cons;
            printLoopStatistics(N_cons, N_mes_update, [&N_cons](){
                std::cout << "Received " << N_cons << " messages" << std::endl;
            });
            if( N_cons >= N_mes )
                break;
        }
    }

    auto t2 = std::chrono::system_clock::now();

    std::chrono::duration<double> comm_seconds = t2 - t1;

    std::cout << "Printing last packet" << std::endl ;

    HeaderWF::print(serializedQueue.back(), 10);
    
    std::cout << std::endl ;

    std::cout << "Consumed number of messages: " << N_mes << std::endl ;
    std::cout << "Communication time : " << comm_seconds.count() << " seconds";
    std::cout << "; " << N_mes/comm_seconds.count() << " packet/s\n";

    delete consumer;
    context.close();

    return 0;
}
