#ifndef SUPERVISORCTRLSERVER_H
#define SUPERVISORCTRLSERVER_H

#include "Supervisor.h"
#include <zmq.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

class SupervisorCtrlServer : public Supervisor {
public:
    // Constructor
    SupervisorCtrlServer(const std::string& config_file = "config.json", const std::string& name = "RTADPCtrlServer");

    ~SupervisorCtrlServer() override;
    // To be reimplemented ####
    // Open the file before loading it into the queue. For "dataflowtype": "file"
    // Return an array of data and the size of the array
    std::pair<std::vector<std::string>, int> open_file(const std::string& filename);
    
    zmq::socket_t* ctrl_socket;

    void stop_custom() override; 
    void start_custom() override; 
};

void buildDefaultA0Packet(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID)  ;
void buildStartAcqPacket(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID) ;

#endif // SUPERVISORCTRLSERVER_H