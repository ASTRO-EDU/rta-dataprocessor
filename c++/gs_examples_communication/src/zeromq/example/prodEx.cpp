#include <random>
#include <fstream>
#include <csignal>
#include <atomic>
#include <condition_variable>

#include "producer.hh"
#include "utils.hh"

const int N_mes = 100;

int main() {
    std::signal(SIGINT, signalHandler);

    std::ifstream config("config.txt");
    std::string ip_port;
    if (config.is_open()) {
        std::getline(config, ip_port);
        config.close();
    } else {
        std::cerr << "Unable to open config file!" << std::endl;
        return 1;
    }

    zmq::context_t context(1);
    Producer<std::vector<uint8_t>> producer(context, "tcp://" + ip_port);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(1, 100);

    
    for (int i = 0; i < N_mes && !stop; ++i) {
        std::vector<uint8_t> randomValues ;
        for (int j = 0 ; j < distr(gen); j++)
            randomValues.push_back(j);

        producer.produce(randomValues);
    }

    return 0;
}
