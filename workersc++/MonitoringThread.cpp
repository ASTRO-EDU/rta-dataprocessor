// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <json/json.h>  // Assuming a suitable JSON library
#include <zmq.hpp>  // Assuming zmq is being used for socket communication

class MonitoringThread {
public:
    // Constructor
    MonitoringThread(zmq::socket_t* socket_monitoring, MonitoringPoint* monitoringpoint)
        : socket_monitoring(socket_monitoring), monitoringpoint(monitoringpoint), _stop_event(false) {
        std::cout << "Monitoring-Thread started" << std::endl;
    }

    // Destructor to ensure the thread is stopped before destruction
    ~MonitoringThread() {
        stop();
        if (monitoring_thread.joinable()) {
            monitoring_thread.join();
        }
    }

    // Start the monitoring thread
    void start() {
        monitoring_thread = std::thread(&MonitoringThread::run, this);
    }

    // Stop the monitoring thread
    void stop() {
        _stop_event = true;
    }

    // Send data to a specific process target name
    void sendto(const std::string& processtargetname) {
        Json::Value monitoring_data = monitoringpoint->get_data();
        monitoring_data["header"]["pidtarget"] = processtargetname;
        Json::StreamWriterBuilder writer;
        std::string monitoring_data_str = Json::writeString(writer, monitoring_data);
        zmq::message_t message(monitoring_data_str.data(), monitoring_data_str.size());
        socket_monitoring->send(message);
        std::cout << "send monitoring" << std::endl;
        std::cout << monitoring_data_str << std::endl;
    }

private:
    zmq::socket_t* socket_monitoring;
    MonitoringPoint* monitoringpoint;
    std::atomic<bool> _stop_event;
    std::thread monitoring_thread;

    // Main run function
    void run() {
        while (!_stop_event) {
            Json::Value monitoring_data = monitoringpoint->get_data();
            Json::StreamWriterBuilder writer;
            std::string monitoring_data_str = Json::writeString(writer, monitoring_data);
            zmq::message_t message(monitoring_data_str.data(), monitoring_data_str.size());
            socket_monitoring->send(message);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

