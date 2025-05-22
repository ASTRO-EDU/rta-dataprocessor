#include "Worker2.h"
#include "Supervisor2.h"
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

std::vector<uint8_t> Worker2::processData(const std::vector<uint8_t>& data, int priority) {
    std::cout << "DENTRO Worker2::processData" << std::endl;

    std::cout << "Worker2::process_data priority:" << priority << std::endl;

    std::cout << "\n RICEZIONE DI Worker2::processData():" << std::endl;

    // Verifica dimensione minima
    if (data.size() < sizeof(int32_t)) {
        std::cerr << "Error: Received data size is smaller than expected." << std::endl;
    }

    // Extract the size of the packet (first 4 bytes)
    int32_t size;
    std::memcpy(&size, data.data(), sizeof(int32_t));  // Read the size from the buffer

    if (size <= 0 || size > static_cast<int32_t>(data.size() - sizeof(int32_t))) {
        std::cerr << "Invalid size value2: " << size << std::endl;
    }

    std::vector<uint8_t> vec(size);
    vec.resize(size);    // Resize the data vector to hold the full payload

    // Store into vec only the actual packet data, excluding the size field
    memcpy(vec.data(), static_cast<const uint8_t*>(data.data()), size);

    // Transform the raw data into the Header struct
    const Header* receivedPacket = reinterpret_cast<const Header*>(vec.data());
    uint32_t packet_type = receivedPacket->type;  // Get the type of the received packet

    // Access the Header fields
    std::cout << "  APID: " << receivedPacket->apid << std::endl;
    std::cout << "  Counter: " << receivedPacket->counter << std::endl;
    std::cout << "  Type: " << receivedPacket->type << std::endl;
    std::cout << "  Absolute Time: " << receivedPacket->abstime << std::endl;

    return {};
}

/*
// Override the process_data method
nlohmann::json Worker2::processData(const nlohmann::json& data, int priority) {

    nlohmann::json result;
    std::string dataflow_type = get_supervisor()->dataflowtype;

    if (dataflow_type == "binary") {
        // Assuming data contains binary data as a string
        std::string binary_data = data.get<std::string>();
        std::unique_ptr<avro::InputStream> in = avro::memoryInputStream(
            reinterpret_cast<const uint8_t*>(binary_data.data()), binary_data.size()
        );

        // Create a binary decoder
        auto decoder = avro::binaryDecoder();
        decoder->init(*in);

        // Use GenericDatum to deserialize data
        avro::GenericDatum datum(avro_schema);
        avro::decode(*decoder, datum);

        if (datum.type() == avro::AVRO_RECORD) {
            const avro::GenericRecord& record = datum.value<avro::GenericRecord>();
            std::string name = record.field("name").value<avro::GenericDatum>().value<std::string>();
            result["name"] = name;
            std::cout << "Deserialized name: " << name << std::endl;
        }

        // Simulate processing
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(random_duration())));
    }
    else if (dataflow_type == "filename") {
        std::string filename = data.get<std::string>();
        // Simulate processing
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(random_duration())));
        result["filename"] = filename;
        std::cout << "Processed file: " << filename << std::endl;
    }
    else if (dataflow_type == "string") {
        std::string str_data = data.get<std::string>();
        result["data"] = str_data;
        std::cout << "Processed string data: " << str_data << std::endl;
    }

    result["priority"] = priority;
    return result;
}
*/

// Helper function to generate random duration between 0 and 100 milliseconds
double Worker2::random_duration() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 100.0);
    return dis(gen);
}
