#include <fstream>
#include <csignal>
#include <atomic>
#include <condition_variable>

#include "consumer.hh"
#include "utils.hh"

const int N_mes = 100;

void printVector(const std::vector<uint8_t>& vec) {
    std::cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        // Print the numeric value of each element
        std::cout << static_cast<int>(vec[i]);
        if (i != vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}


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
    Consumer<std::vector<uint8_t>> consumer(context, "tcp://" + ip_port);

    int N_cons = 0;
    

    while (N_cons < N_mes && !stop) {
        std::vector<uint8_t> value;
        if (consumer.consume(value)) {

            printVector(value);

            ++N_cons;
            if (N_cons % 1000000 == 0) {
                std::cout << "Consumed " << N_cons << " messages." << std::endl;
            }
        }
    }

    return 0;
}
