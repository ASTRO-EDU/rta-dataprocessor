#include "Worker2.h"
#include "Supervisor.h"
#include "avro/Generic.hh"
#include "avro/Schema.hh"
#include "avro/ValidSchema.hh"
#include "avro/Compiler.hh"
#include "avro/GenericDatum.hh"
#include "avro/DataFile.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"
#include "ccsds/include/packet.h"
#include <iostream>

// Constructor
Worker2::Worker2() : WorkerBase() {

}

// Override the config method
void Worker2::config(const nlohmann::json& configuration) {
    WorkerBase::config(configuration);
}

// Process the data extracted from the queue
std::vector<uint8_t> Worker2::processData(const std::vector<uint8_t>& data, int priority) {
    std::vector<uint8_t> binary_result;    
    std::string dataflow_type = get_supervisor()->dataflowtype;

    if (dataflow_type == "binary") {
        // Check minimum dimension
        if (data.size() < sizeof(int32_t)) {
            std::cerr << "[Worker2] Error: Received data size is smaller than expected." << std::endl;
            return binary_result; // Return an empty vector
        }

        // Extract the size of the packet (first 4 bytes)
        int32_t size;
        std::memcpy(&size, data.data(), sizeof(int32_t));

        // Size has to be non-negative and does not exceed the available data in data
        if (size <= 0 || size > data.size() - sizeof(int32_t)) {
            std::cerr << "[Worker2] Invalid size value: " << size << std::endl;
        }

        std::vector<uint8_t> vec(size);
        vec.resize(size);    // Resize the data vector to hold the full payload

        // Store into vec only the actual packet data, excluding the size field
        //memcpy(vec.data(), static_cast<const uint8_t*>(data.data()), size);
        memcpy(vec.data(), static_cast<const uint8_t*>(data.data()) + sizeof(int32_t), size);

        const uint8_t* raw_data = vec.data();

        // Extract payload in order to get the packet type
        const Data_HkDams* receivedPayload = reinterpret_cast<const Data_HkDams*>(raw_data + sizeof(HeaderDams));
        uint8_t packet_type = receivedPayload->type;  // Store type in a variable

        if (packet_type == Data_WaveData::TYPE) {  // WF Packet
            std::cout << "[Worker2] Waveform packet received. Printing infos: " << std::endl;
        }
        else if (packet_type == Data_HkDams::TYPE) { // HK Packet
            std::cout << "[Worker2] Housekeeping packet received. Printing infos: " << std::endl;
        }

        // Extract the header of the packet to print some infos
        const HeaderDams* receivedHeader = reinterpret_cast<const HeaderDams*>(raw_data);
        uint8_t start = receivedHeader->start;
        uint8_t apid = receivedHeader->apid;
        uint16_t sequence = receivedHeader->sequence;
        uint16_t runID = receivedHeader->runID;
        uint16_t header_size = receivedHeader->size;
        uint32_t crc = receivedHeader->crc;

        std::cout << "Header:" << std::endl;
        std::cout << "  Start Byte: " << std::hex << (int)start << std::endl;
        std::cout << "  APID: " << std::hex << (int)apid << std::endl;
        std::cout << "  Sequence: " << std::hex << sequence << std::endl;
        printf("  Run ID: %04X\n", runID);
        std::cout << "  Size: " << std::dec << (int)header_size << std::endl;
        printf("  CRC: %08X\n", crc);
        std::cout << "Payload:" << std::endl;
        std::cout << "  Type: " << std::hex << (int)packet_type << std::endl;

        // Payload to return
        // binary_result.insert(binary_result.end(), data.begin(), data.end());  // Append data at the end
        binary_result.insert(binary_result.end(), vec.begin(), vec.end());  // Append data at the end
    }
    else if (dataflow_type == "filename") {

    }
    else if (dataflow_type == "string") {

    }

    return binary_result;
}
