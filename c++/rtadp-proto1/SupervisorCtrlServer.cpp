#include "./include/SupervisorCtrlServer.h"
#include "ccsds/include/packet.h"
#include "../include/utils2.hh"

static const size_t buffSz = 128;
// Constructor
SupervisorCtrlServer::SupervisorCtrlServer(const std::string& config_file, const std::string& name)
    : Supervisor(config_file, name) {

    try{
        ctrl_socket = new zmq::socket_t(context, ZMQ_PUSH);
        // If the pipeline is running on the Jetson use 192.168.166.127, if it is running on a local machine then use 127.0.0.1
        std::string ctrl_address = "tcp://169.254.158.233:1235";  
        ctrl_socket->connect(ctrl_address);
        logger->info("[SupervisorCtrlServer] Control socket connected to: " + ctrl_address, globalname);
        std::cout << "[SupervisorCtrlServer] Control socket connected to: " << ctrl_address << std::endl;
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
    std::cout << "[SupervisorCtrlServer] Cleaning up SupervisorCtrlServer resources..." << std::endl;
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
    
    uint8_t buff[buffSz];

    buildStartAcqPacket(buff, buffSz, 0x01);
    zmq::message_t msg(buff, buffSz);
    ctrl_socket->send(msg, zmq::send_flags::none);
    logger->info("[SupervisorCtrlServer] Sent control command: StartAcqPacket");
}


void SupervisorCtrlServer::stop_custom() {
    // We send a stop signal to the listening producer (gfse.py) in order for it to stop sending data
    uint8_t buff[buffSz];
    buildStopAcqPacket(buff, buffSz, 0x01);
    zmq::message_t msg(buff, buffSz);
    ctrl_socket->send(msg, zmq::send_flags::none);
    logger->info("[SupervisorCtrlServer] Sent control command: StartAcqPacket");
    
}

void buildDefaultA0Packet(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID = 0) {
    if (maxBufferSize < sizeof(Header) + sizeof(Data_Header)) {
        printf("Error: buffer too small in buildDefaultA0Packet\n");
        return;
    }

    HeaderDams* header = (HeaderDams*)buffer;
    header->start = HeaderDams::START;
    header->apid = HeaderDams::CLASS_TC | 0x01;  // SOURCE = 1 esempio
    header->sequence = HeaderDams::GROUP_STAND_ALONE | 0x0001; // esempio count 1
    header->runID = runID;
    header->size = sizeof(Data_Header); // dati utili dopo header
    header->crc = 0; // se hai CRC calcolalo poi!
    header->encode();

    Data_Header* data = (Data_Header*)(buffer + sizeof(HeaderDams));
    data->type = 0xA0;
    data->subType = 0x99; // caso "non definito" ? farà trigger FE
}

void buildStartAcqPacket(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID = 0) {
    if (maxBufferSize < sizeof(HeaderDams) + sizeof(Data_StartAcq)) {
        printf("Error: buffer too small in buildStartAcqPacket\n");
        return;
    }

    HeaderDams* header = (HeaderDams*)buffer;
    header->start = HeaderDams::START;         // 0x8D
    header->apid = HeaderDams::CLASS_TC | 0x0A; // 0x0A 
    header->sequence = HeaderDams::GROUP_STAND_ALONE | 0x0000;
    header->runID = 0x0000;
    header->size = sizeof(Data_StartAcq);
    header->crc = 0;
    header->encode(); // se encode() fa altro, ok; se no, puoi anche omettere

    Data_StartAcq* data = (Data_StartAcq*)(buffer + sizeof(HeaderDams));
    data->type = Data_StartAcq::TYPE;
    data->subType = Data_StartAcq::SUB_TYPE;
    data->source = 0;
    data->spare0 = 0;
    data->maxWaveNo = 0;
    data->waitUsecs = 0;
}

void buildStopAcqPacket(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID = 0) {
    if (maxBufferSize < sizeof(HeaderDams) + sizeof(Data_Header)) {
        printf("Error: buffer too small in buildStartAcqPacket\n");
        return;
    }

    HeaderDams* header = (HeaderDams*)buffer;
    header->start = HeaderDams::START;
    header->apid = HeaderDams::CLASS_TC | 0x01;  // SOURCE = 1 esempio
    header->sequence = HeaderDams::GROUP_STAND_ALONE | 0x0002; // esempio count 2
    header->runID = runID;
    header->size = sizeof(Data_Header);
    header->crc = 0; // se hai CRC calcolalo poi!
    header->encode();

    Data_Header* data = (Data_Header*)(buffer + sizeof(HeaderDams));
    data->type = 0xA0;
    data->subType = 0x05; // StopAcq
}

