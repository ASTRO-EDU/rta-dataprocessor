#include "Worker1.h"
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
Worker1::Worker1() : WorkerBase() {
    // Load Avro schema from the provided schema string
    std::string avro_schema_str = R"({
        "type": "record",
        "name": "AvroMonitoringPoint",
        "namespace": "astri.mon.kafka",
        "fields": [
            {"name": "assembly", "type": "string"},
            {"name": "name", "type": "string"},
            {"name": "serial_number", "type": "string"},
            {"name": "timestamp", "type": "double"},
            {"name": "source_timestamp", "type": ["null", "long"]},
            {"name": "units", "type": "string"},
            {"name": "archive_suppress", "type": "boolean"},
            {"name": "env_id", "type": "string"},
            {"name": "eng_gui", "type": "boolean"},
            {"name": "op_gui", "type": "boolean"},
            {"name": "data", "type": {"type": "array", "items": ["double", "int", "long", "string", "boolean"]}}
        ]
    })";

    std::istringstream schema_stream(avro_schema_str);
    avro::compileJsonSchema(schema_stream, avro_schema);

    this->avro_schema = avro_schema;
}

// Override the config method
void Worker1::config(const nlohmann::json& configuration) {
    WorkerBase::config(configuration);
}

std::string get_current_time_as_string() {
    auto now = std::chrono::system_clock::now();  // Tempo corrente
    auto now_time_t = std::chrono::system_clock::to_time_t(now); // Convertito a time_t
    auto local_time = *std::localtime(&now_time_t); // Ottieni il tempo locale

    // Usa std::ostringstream per convertire la data e ora in una stringa
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S"); // Formato leggibile
    return oss.str();
}

////////////////////////////////////////////
std::vector<uint8_t> Worker1::processData(const std::vector<uint8_t>& data, int priority) {
    std::vector<uint8_t> binary_result;    
    std::string dataflow_type = get_supervisor()->dataflowtype;

    if (dataflow_type == "binary") {
        // Check minimum dimension
        if (data.size() < sizeof(int32_t)) {
            std::cerr << "Error: Received data size is smaller than expected." << std::endl;
            return binary_result; // Return an empty vector
        }

        // Extract the size of the packet (first 4 bytes)
        int32_t size;
        std::memcpy(&size, data.data(), sizeof(int32_t));  

        // Size has to be non-negative and does not exceed the available data in data
        if (size <= 0 || size > data.size() - sizeof(int32_t)) {
            std::cerr << "Invalid size value: " << size << std::endl;
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
            std::cout << "Waveform packet received. Printing infos: " << std::endl;
        }
        else if (packet_type == Data_HkDams::TYPE) { // HK Packet
            std::cout << "Housekeeping packet received. Printing infos: " << std::endl;
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
        printf("  Size: %04X\n", header_size);
        printf("  CRC: %08X\n", crc);

        std::cout << "Payload:" << std::endl;
        std::cout << "  Type: " << std::dec << (int)packet_type
            << " (0x" << std::hex << (int)packet_type << ")" << std::endl;

        // Payload to return
        // binary_result.insert(binary_result.end(), data.begin(), data.end());  // Append data at the end
        binary_result.insert(binary_result.end(), vec.begin(), vec.end());  // Append data at the end
    } 
    else if (dataflow_type == "filename") {
        nlohmann::json result;

        const std::string filename(data.begin(), data.end());
        // Simulate processing
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(random_duration())));
        std::cout << "Processed file: " << filename << std::endl;

        result["filename"] = filename;

        std::string current_time = get_current_time_as_string();
        result["timestamp"] = current_time;

        std::string json_str = result.dump();
        binary_result = std::vector<uint8_t>(json_str.begin(), json_str.end());
    }
    else if (dataflow_type == "string") {
        nlohmann::json result;

        const std::string str_data(data.begin(), data.end());
        std::cout << "\nProcessed string data: " << str_data << std::endl;

        result["data"] = str_data;

        std::string current_time = get_current_time_as_string();
        result["timestamp"] = current_time;

        std::string json_str = result.dump();
        binary_result = std::vector<uint8_t>(json_str.begin(), json_str.end());

        std::cout << "binary_result: " << binary_result.size() << std::endl;
    }

    return binary_result;
}
////////////////////////////////////////////

// Helper function to generate random duration between 0 and 100 milliseconds
double Worker1::random_duration() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 100.0);
    return dis(gen);
}
