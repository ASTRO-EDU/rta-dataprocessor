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
void Worker2::config(const nlohmann::json& configuration) {
    WorkerBase::config(configuration);
}

std::vector<uint8_t> Worker2::processData(const std::vector<uint8_t>& data, int priority) {
    std::cout << "DENTRO Worker2::processData" << std::endl;

    std::cout << "\n RICEZIONE DI Worker2::processData():" << std::endl;

    // Verifica dimensione minima
    if (data.size() < sizeof(int32_t)) {
        std::cerr << "Error: Received data size is smaller than expected." << std::endl;
    }

    // Estrai la dimensione del payload
    int32_t size;
    std::memcpy(&size, data.data(), sizeof(int32_t));  // Read the size from the buffer

    if (size <= 0 || size > static_cast<int32_t>(data.size() - sizeof(int32_t))) {
        std::cerr << "Invalid size value: " << size << std::endl;
    }

    const HeaderWF* receivedPacket = reinterpret_cast<const HeaderWF*>(data.data());
    // std::memcpy(&receivedPacket, vec.data(), sizeof(HeaderWF));
    // std::cout << "Ci sono4" << std::endl;

    // Verify the content of the debufferized data
    std::cout << "Debufferized Header APID: " << receivedPacket->h.apid << std::endl;
    std::cout << "Debufferized Data size: " << receivedPacket->d.size << std::endl;
    std::cout << "Size of timespec: " << sizeof(receivedPacket->h.ts) << ", Alignment:" << alignof(receivedPacket->h.ts) << "\n" << std::endl;

    HeaderWF::print(*receivedPacket, 10);

    /*
    std::string str(data.begin(), data.end());
    nlohmann::json result = nlohmann::json::parse(str);

    if (result.is_array() && !result.empty()) {
        std::cout << "Worker2::processData: TIMESTAMP " << result[0]["timestamp"] << std::endl;
    }
    else if (result.is_object()) {
        std::cout << "Worker2::processData: TIMESTAMP " << result["timestamp"] << std::endl;
    }
    */

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
