#ifndef SUPERVISOR2_H
#define SUPERVISOR2_H

#include "SupervisorCtrlServer.h"
#include "WorkerManager2.h"
#include <zmq.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

class Supervisor2 : public SupervisorCtrlServer {
public:
    // Constructor
    Supervisor2(const std::string& config_file = "config.json", const std::string& name = "RTADP2");

    ~Supervisor2() override;
    
    // Override the start_managers method
    void start_managers();

    // Override listen_for_lp_data to handle DAMS packets
    void listen_for_lp_data();

    // To be reimplemented ####
    // Open the file before loading it into the queue. For "dataflowtype": "file"
    // Return an array of data and the size of the array
    std::pair<std::vector<std::string>, int> open_file(const std::string& filename);
};

#endif // SUPERVISOR2_H
