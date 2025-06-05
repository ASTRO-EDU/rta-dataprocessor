#include "SupervisorCtrlServer.h"
#include "ccsds/include/packet.h"
#include "../include/utils2.hh"

// Constructor
SupervisorCtrlServer::SupervisorCtrlServer(const std::string& config_file, const std::string& name)
    : Supervisor(config_file, name) {

    try{
        ctrl_socket = new zmq::socket_t(context, ZMQ_PUSH);
        // If the pipeline is running on the Jetson use 192.168.166.127, if it is running on a local machine then use 127.0.0.1
        std::string ctrl_address = "tcp://127.0.0.1:1235";  
        ctrl_socket->connect(ctrl_address);
        logger->info("[SupervisorCtrlServer] Control socket connected to: " + ctrl_address, globalname);
    }
    catch (const std::exception& e) {
        // Handle any other unexpected exceptions
        std::cerr << "[SupervisorCtrlServer] ERROR: An unexpected error occurred: " << e.what() << std::endl;
        logger->warning("[SupervisorCtrlServer] ERROR: An unexpected error occurred: " + std::string(e.what()), globalname);
        exit(1);
    }
}

// Destructor
SupervisorCtrlServer::~SupervisorCtrlServer() {
    if (ctrl_socket) {
        try {
            ctrl_socket->close();
        }
        catch (const zmq::error_t& e) {
            logger->error("Error while closing ctrl_socket: {}", e.what());
        }
        delete ctrl_socket;
        ctrl_socket = nullptr;
    }
}

void SupervisorCtrlServer::start_custom() {

    // We send a start signal to the listening producer (gfse.py) in order for it to start sending data
    
    std::string command = "START"; 
    zmq::message_t msg(command.data(), command.size());
    
    ctrl_socket->send(msg, zmq::send_flags::none);
    logger->info("[SupervisorCtrlServer] Sent control command: ", command);
    
}


void SupervisorCtrlServer::stop_custom() {
    // We send a stop signal to the listening producer (gfse.py) in order for it to stop sending data
    
    std::string command = "STOP"; 
    zmq::message_t msg(command.data(), command.size());
    ctrl_socket->send(msg, zmq::send_flags::none);
    logger->info("[Supervisor] Sent control command: ", command);
    
}
