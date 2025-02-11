#ifndef GS_COMM_UTILS_HH
#define GS_COMM_UTILS_HH
#include <atomic>
#include <string>

#include <zmq.hpp>

#include <functional>

#define N_MES_DEF 1
#define N_MES_UPDATE_SCALER 5

extern std::atomic<bool> stop;

void signalHandler(int signal);

const int parseArguments( int argc,  char* argv[]);

const int getMessageUpdate(const int N_mes);

std::string getIpPortFromConfig(const std::string& filename);

bool receiveMessage(zmq::socket_t& socket, std::function<void(const zmq::message_t&)> processMessage);

bool sendMessage(zmq::socket_t& socket, std::function<zmq::message_t()> prepareMessage);

void printMemoryUsage();

void printLoopStatistics(size_t len, const int N_mes_update, std::function<void()> printMessage);

std::pair<int, int> minmaxMessageSizes();

#endif