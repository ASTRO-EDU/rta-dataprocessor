#include "Supervisor1.h"

#include "ccsds/include/packet.h"
#include "../include/utils2.hh"

// Constructor
Supervisor1::Supervisor1(const std::string& config_file, const std::string& name) 
    : Supervisor(config_file, name) {
}

// Destructor
Supervisor1::~Supervisor1() {
    for (WorkerManager* m: manager_workers)
        delete m;
}
    
// Override the start_managers method
void Supervisor1::start_managers() {
    int indexmanager = 0;
    WorkerManager* manager1 = new WorkerManager1(indexmanager, this, workername);
    setup_result_channel(manager1, indexmanager);
    manager1->run();
    manager_workers.push_back(manager1);
    logger->info("DER SUP1 manager started");
}

///////////////////////////////////////////////////////////////////
// Override listen_for_lp_data to handle DAMS packets
void Supervisor1::listen_for_lp_data() {
    while (continueall) {
        if (!stopdata) {
            zmq::message_t data;
            zmq::recv_flags flags = zmq::recv_flags::none;

            try {
                auto result = socket_lp_data->recv(data, flags);
                int err_code = zmq_errno();

                if (!result) {
                    // std::cout << "Waiting for a producer" << std::endl;

                    /*
                    while (err_code == EAGAIN) {   // Continue if no commands were received
                        // std::cout << "Waiting" << std::endl;
                        continue; // Keep looking for commands
                    }
                    // continue; // Keep looking for commands
                    */
                }
            }
            catch (const zmq::error_t& e) {
                int err_code = zmq_errno();

                if (err_code == EINTR) {     // SIGINT
                    break;
                }
                else {
                    std::cerr << "[Supervisor1] ZMQ exception in listen_for_lp_data: " << e.what() << std::endl;
                    logger->error("[Supervisor1] ZMQ exception in listen_for_lp_data: {}", e.what());
                    throw;
                }
            }


            if (data.size() < sizeof(int32_t)) {
                // std::cerr << "[Supervisor1] ERROR: Packet too small to contain size prefix" << std::endl;
                continue;  // skip to next packet
            }

            int32_t size;
            memcpy(&size, data.data(), sizeof(int32_t));

            // Verify that the size prefix is positive and matches the actual payload size. The total message should be exactly 4 bytes (prefix) 
            // + "size" bytes (payload).
            if (size <= 0 || size != static_cast<int32_t>(data.size() - sizeof(uint32_t))) {
                std::cerr << "[Supervisor1] Invalid size value: " << size << std::endl;
                continue;
            }
            // std::cout << "[Supervisor1] Extracted packet size: " << std::dec << (int)size << " (0x" << std::hex << (int)size << ")" << std::endl;


            // std::cout << "Received Raw Packet: ";
            const uint8_t* raw_packet = static_cast<const uint8_t*>(data.data());
            /*
            for (size_t i = 0; i < data.size(); ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)raw_packet[i] << " ";
            }
            std::cout << std::dec << std::endl;
            */

            uint8_t packet_type = raw_packet[4 + sizeof(HeaderDams)]; // 4 bytes for size + header bytes (gs_examples_communication/gs_examples_serialization/ccsds/include/packet.h)
            uint8_t subtype = raw_packet[4 + sizeof(HeaderDams) + 1];
            // [4 bytes size prefix]
            // [12 bytes HeaderDams] 
            // [44 bytes Data_WaveHeader]

            // std::cout << "TYPE: " << std::hex << static_cast<int>(packet_type) << ", SUBTYPE: " << static_cast<int>(subtype) << std::dec << std::endl;

            if (packet_type == Data_WaveHeader::TYPE) {  // WF Packet
                // std::cout << "[Supervisor1] Waveform packet received. Pushing into the queue" << std::endl;

                // const WfPacketDams* packet_wf = reinterpret_cast<const WfPacketDams*>(raw_packet + sizeof(uint32_t));

                const uint8_t* rp = raw_packet;

                // Cast to header+waveheader only 
                const HeaderDams* h_ptr = reinterpret_cast<const HeaderDams*>(rp + 4);  // The generic header starts after the first 4 bytes (which contain the size)
                const Data_WaveHeader* w_ptr = reinterpret_cast<const Data_WaveHeader*>(rp + 4 + sizeof(HeaderDams));   // Then follows Data_WaveHeader

                // At the moment the true area size is stored inside the usec field of Data_WaveHeader for simplicity in order to be compared to the predicted one
                std::cout << "\n[Supervisor1] REAL AREA: " << w_ptr->us << std::endl;   

                // Manually build the WfPacketDams (the full waveform packet)
                WfPacketDams full_packet;
                full_packet.body.h = *h_ptr;
                full_packet.body.w = *w_ptr;
                full_packet.body.d.type = Data_WaveData::TYPE;
                full_packet.body.d.subType = Data_WaveData::SUB_TYPE;
                // The waveform buffer (actual waveform values that will be handled in Worker1)
                std::memcpy(full_packet.body.d.buff, rp + 4 + sizeof(HeaderDams) + sizeof(Data_WaveHeader), U32_X_PACKET * sizeof(uint32_t));

                // Push into the queue the serialized packet
                for (auto& manager : manager_workers) {
                    manager->getLowPriorityQueue()->push(serializePacket(full_packet));
                }

                // std::cout << "Finished pushing into the queue" << std::endl;
            }
            else if (packet_type == Data_HkDams::TYPE) {  // HK Packet
                std::cout << "[Supervisor1] Housekeeping packet received." << std::endl;
            }
            else {
                std::cout << "[Supervisor1] Unknown packet type: " << packet_type << std::endl;
            }
        }
    }

    std::cout << "[Supervisor1] End listen_for_lp_data" << std::endl;
    logger->info("[Supervisor1] End listen_for_lp_data", globalname);
}
///////////////////////////////////////////////////////////////////

// For "dataflowtype": "binary", decode the data before loading it into the queue.
zmq::message_t& Supervisor1::decode_data(zmq::message_t& data) {
    return data;
}

// For "dataflowtype": "file", open the file before loading it into the queue. 
// Return an array of data and the size of the array
std::pair<std::vector<std::string>, int> Supervisor1::open_file(const std::string& filename) {
    std::vector<std::string> f = {filename};
    return {f, 1};
}
