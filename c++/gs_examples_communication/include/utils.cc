#include <csignal>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sys/resource.h>

#include "utils.hh"


std::atomic<bool> stop{false}; 

void signalHandler(int signal) {
    if (signal == SIGINT) {
        stop.store(true);
        std::cout << "\nSIGINT received. Setting stop flag." << std::endl;
    }
}

const int parseArguments(int argc, char* argv[]) {
    int N_mes = N_MES_DEF;

    if (argc > 1) {
        try {
            int got = std::stoi(argv[1]);  // Convert the argument to an integer
            if (got < 1) throw std::invalid_argument("N_mes must be greater or equal to 1");
            else N_mes = got;
        } catch (const std::invalid_argument& e) {
            std::cerr << "Invalid argument: " << argv[1] << ". Using default value of " << N_mes << "." << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Argument out of range: " << argv[1] << ". Using default value of " << N_mes << "." << std::endl;
        }
    } else {
        std::cout << "No argument provided. Using default value of " << N_mes << "." << std::endl;
    }

    return N_mes;
}

const int getMessageUpdate(const int N_mes){
    double res = N_mes / (float) N_MES_UPDATE_SCALER;
    if (res <= 1)
        return 1;
    else
        return (int) res;
}

// Function to read the IP and port from a config file
std::string getIpPortFromConfig(const std::string& filename) {
    std::ifstream config(filename);
    std::string ip_port;

    if (config.is_open()) {
        std::getline(config, ip_port);
        config.close();
    } else {
        std::cerr << "Unable to open config file: " << filename << std::endl;
    }

    return ip_port;
}

bool handleZmqOperation(std::function<bool()> operation)
{
    try
    {
        return operation();
    }
    catch (const zmq::error_t& e)
    {
        if (e.num() == EINTR) // Interrupted system call
        {
            std::cerr << "Operation interrupted by signal." << std::endl;
            return false;
        }
        else
        {
            std::cerr << "ZeroMQ error: " << e.what() << std::endl;
            throw; // Re-throw other errors
        }
    }
}

bool receiveMessage(zmq::socket_t& socket, std::function<void(const zmq::message_t&)> processMessage)
{
    zmq::message_t message;
    return handleZmqOperation([&]() -> bool
    {
        if (socket.recv(message, zmq::recv_flags::none))
        {
            processMessage(message);
            return true;
        }
        return false;
    });
}

bool sendMessage(zmq::socket_t& socket, std::function<zmq::message_t()> prepareMessage)
{
    return handleZmqOperation([&]() -> bool
    {
        auto message = prepareMessage();
        socket.send(message, zmq::send_flags::none);
        return true;
    });
}

void printLoopStatistics( size_t len, const int N_mes_update, std::function<void()> printMessage) {
    if (len % N_mes_update == 0) {
        printMessage();
        printMemoryUsage();
    }
}

void printMemoryUsage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        std::cout << "Max resident set size: " << usage.ru_maxrss << " KB" << std::endl;
    } else {
        std::cerr << "Failed to get memory usage." << std::endl;
    }
}
