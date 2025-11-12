#ifndef MONITORINGTHREAD_H
#define MONITORINGTHREAD_H

#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <string>
#include <rtadp/json.hpp>
#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <rtadp/MonitoringPoint.h>

class MonitoringPoint; // Forward declaration


class MonitoringThread {  
    std::thread thread;  // The monitoring thread
    std::atomic<bool> stop_event;  // Atomic flag to stop the thread
    zmq::socket_t& socket_monitoring;  // Reference to the ZMQ socket for sending data
    MonitoringPoint& monitoringpoint;  // Reference to the MonitoringPoint
  
public:
    // Constructor to initialize the MonitoringThread with a socket and MonitoringPoint reference
    MonitoringThread(zmq::socket_t& socket_monitoring, MonitoringPoint& monitoringpoint);
    
    // Destructor to stop the thread and clean up resources
    ~MonitoringThread();

    void start();
    void stop();
    void run();


    // Sends monitoring data to a specific process target name
    void sendto(const std::string& processtargetname);
};

#endif // MONITORINGTHREAD_H
