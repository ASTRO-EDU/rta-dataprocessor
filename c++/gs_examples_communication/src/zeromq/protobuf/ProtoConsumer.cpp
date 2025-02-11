#include <fstream>
#include <csignal>
#include <atomic>
#include <condition_variable>
#include <queue>

#include "WFGenerator.hh"
#include "ProtocolBufferEntities.hh"

#include "consumer.hh"
#include "utils.hh"

std::vector<HeaderandWaveform> results;

int main(int argc, char* argv[]) {

    const int N_mes = parseArguments(argc, argv);
    const int N_mes_update = getMessageUpdate(N_mes);
    std::cout << "message update every " << N_mes_update << " msgs" << std::endl;

    std::string ip_port = getIpPortFromConfig("config.txt");
 
    std::queue<std::string> serializedQueue;
    MessageReader<HeaderandWaveform> reader(serializedQueue);

    std::cout << "Start Receiving " << std::endl;

    zmq::context_t context(1);
    Consumer<std::string>* consumer = new Consumer<std::string>(context, "tcp://" + ip_port);
    
    int N_cons = 0;
    bool first = true;
    std::chrono::_V2::system_clock::time_point t1;
    while (!stop) {
        std::string value;
        if (consumer->consume(value)) {
            if (first){
                first = false;
                t1 = std::chrono::system_clock::now();
            }
            serializedQueue.push(value);  // Push the consumed value into the queue
            ++N_cons;
            printLoopStatistics(N_cons, N_mes_update, [&N_cons](){
                std::cout << "Received " << N_cons << " messages" << std::endl;
            });
            if( N_cons >= N_mes )
                break;
        }
    }

    auto t2 = std::chrono::system_clock::now();

    std::cout << "Start Deserializing " << std::endl;

    N_cons = 0;
    while (!stop) {
        results.push_back(reader.readMessage());
            
        printLoopStatistics(serializedQueue.size(), N_mes_update, [&serializedQueue](){
                std::cout << "Remaining " << serializedQueue.size() << " packets" << std::endl;
            });
        // std::cout <<  "Received packet with run id: "<< results.back().runID << ", decimation: " << results.back().decimation << " and pc: " << results.back().counter << std::endl;
        if (serializedQueue.size() <= 0) 
            break; 
    }

    std::cout << "Done" << std::endl;

    auto t3 = std::chrono::system_clock::now();

    std::cout << "Printing last packet" << std::endl ;

    print_WF(results.back(), 10);
    
    std::cout << std::endl ;
    
    std::chrono::duration<double> comm_seconds = t2 - t1;
    std::chrono::duration<double> des_seconds = t3 - t2;

    std::cout << "Consumed number of messages: " << N_mes << std::endl ;
    std::cout << "Communication time : " << comm_seconds.count() << " seconds";
    std::cout << "; " << N_mes/comm_seconds.count() << " packet/s\n";
    std::cout << "Deserialization time : " << des_seconds.count() << " seconds";
    std::cout << "; " << N_mes/des_seconds.count() << " packet/s\n";
    std::cout << "Total Receiving time : " << des_seconds.count() + comm_seconds.count() << " seconds";
    std::cout << "; " << N_mes/(des_seconds.count() + comm_seconds.count()) << " packet/s\n";

    delete consumer;
    context.close();

    return 0;
}
