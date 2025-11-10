#include "include/Supervisor2.h"
#include "packet.h"
#include "utils2.hh"

// Constructor
Supervisor2::Supervisor2(const std::string& config_file, const std::string& name)
    : SupervisorCtrlServer(config_file, name) {
}

// Destructor
Supervisor2::~Supervisor2() {
    for (WorkerManager* m: manager_workers)
        delete m;
}

// Override the start_managers method
void Supervisor2::start_managers() {
    int indexmanager = 0;
    WorkerManager* manager2 = new WorkerManager2(indexmanager, this, workername);
    setup_result_channel(manager2, indexmanager);
    manager2->run();
    manager_workers.push_back(manager2);
    logger->info("[Supervisor2] DER SUP2 manager started");
}

// Override listen_for_lp_data to handle DAMS packets
void Supervisor2::listen_for_lp_data() {
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
                    */
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));     // No data received, sleep to reduce CPU usage
                    continue; // Keep looking for commands
                }
                else {
                    if (data.size() < sizeof(int32_t)) {
                        // std::cerr << "[Supervisor2] ERROR: Packet too small to contain size prefix" << std::endl;
                        continue;  // skip to next packet
                    }

                    int32_t size;
                    memcpy(&size, data.data(), sizeof(int32_t));

                    // Verify that the size prefix is positive and matches the actual payload size. The total message should be exactly 4 bytes (prefix) 
                    // + "size" bytes (payload).
                    if (size <= 0 || size != static_cast<int32_t>(data.size() - sizeof(uint32_t))) {
                        std::cerr << "[Supervisor2] Invalid size value: " << size << std::endl;
                        continue;
                    }

                    //std::cout << "Extracted packet size: " << std::dec << (int)size << " (0x" << std::hex << (int)size << ")" << std::endl;

                    // std::cout << "Received Raw Packet: ";
                    const uint8_t* raw_packet = static_cast<const uint8_t*>(data.data());

                    uint8_t packet_type = raw_packet[4 + sizeof(HeaderDams)]; // 4 bytes for size + header bytes (gs_examples_communication/gs_examples_serialization/ccsds/include/packet.h)
                    uint8_t subtype = raw_packet[4 + sizeof(HeaderDams) + 1];
                    // [4 bytes size prefix]
                    // [12 bytes HeaderDams] 
                    // [44 bytes Data_WaveHeader]

                    // std::cout << "TYPE: " << std::hex << static_cast<int>(packet_type) << ", SUBTYPE: " << static_cast<int>(subtype) << std::dec << std::endl;

                    if (packet_type == Data_WaveHeader::TYPE) {  // WF Packet
                        // Extract the WfPacketDams struct from the raw bytesAdd commentMore actions
                        const WfPacketDams* packet_wf = reinterpret_cast<const WfPacketDams*>(raw_packet + sizeof(uint32_t));

                        for (auto& manager : manager_workers) {
                            manager->getLowPriorityQueue()->push(serializePacket(*packet_wf));
                        }
                    }
                    else if (packet_type == Data_HkDams::TYPE) {  // HK Packet
                        std::cout << "[Supervisor2] Housekeeping packet received" << std::endl;
                    }
                    else {
                        std::cout << "[Supervisor2] Unknown packet type: " << packet_type << std::endl;
                    }
                }
            }
            catch (const zmq::error_t& e) {
                int err_code = zmq_errno();

                if (err_code == EINTR) {     // SIGINT
                    break;
                }
                else {
                    std::cerr << "[Supervisor2] ZMQ exception in listen_for_lp_data: " << e.what() << std::endl;
                    logger->error("[Supervisor2] ZMQ exception in listen_for_lp_data: {}", e.what());
                    throw;
                }
            }
        }
        else {
            // Data processing stopped, sleep to reduce CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "[Supervisor2] End listen_for_lp_data" << std::endl;
    logger->info("[Supervisor2] End listen_for_lp_data", globalname);
}

// For "dataflowtype": "file", open the file before loading it into the queue. 
// Return an array of data and the size of the array
std::pair<std::vector<std::string>, int> Supervisor2::open_file(const std::string& filename) {
    std::vector<std::string> f = {filename};
    return {f, 1};
}
