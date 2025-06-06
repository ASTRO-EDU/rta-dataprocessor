#ifndef SUPERVISOR1_H
#define SUPERVISOR1_H

#include "Supervisor.h"
#include "WorkerManager1.h"
#include <zmq.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

class Supervisor1 : public Supervisor {
public:
    // Constructor
    Supervisor1(const std::string& config_file = "config.json", const std::string& name = "RTADP1");

    ~Supervisor1();

    // Override the start_managers method
    void start_managers();

    // Override listen_for_lp_data to handle DAMS packets
    void listen_for_lp_data();

    // To be reimplemented ####
    // Open the file before loading it into the queue. For "dataflowtype": "file"
    // Return an array of data and the size of the array
    std::pair<std::vector<std::string>, int> open_file(const std::string& filename);
};

#endif // SUPERVISOR1_H