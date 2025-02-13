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

/*
void Worker1::printGenericDatum(const avro::GenericDatum& datum, int indent = 0) {
    // Funzione per aggiungere spazi per l'indentazione
    auto indentSpaces = [](int level) -> std::string {
        return std::string(level * 4, ' ');
        };

    // Aggiungi indentazione
    std::cout << indentSpaces(indent);

    // Controlla il tipo del dato e stampalo
    switch (datum.type()) {
    case avro::AVRO_NULL:
        std::cout << "null" << std::endl;
        break;

    case avro::AVRO_BOOL:
        std::cout << "bool: " << datum.value<bool>() << std::endl;
        break;

    case avro::AVRO_INT:
        std::cout << "int: " << datum.value<int32_t>() << std::endl;
        break;

    case avro::AVRO_LONG:
        std::cout << "long: " << datum.value<int64_t>() << std::endl;
        break;

    case avro::AVRO_FLOAT:
        std::cout << "float: " << datum.value<float>() << std::endl;
        break;

    case avro::AVRO_DOUBLE:
        std::cout << "double: " << datum.value<double>() << std::endl;
        break;

    case avro::AVRO_STRING:
        std::cout << "string: " << datum.value<std::string>() << std::endl;
        break;

    case avro::AVRO_BYTES: {
        std::cout << "bytes: ";
        const std::vector<uint8_t>& bytes = datum.value<std::vector<uint8_t>>();
        for (uint8_t byte : bytes) {
            std::cout << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
        break;
    }

    case avro::AVRO_ARRAY: {
        std::cout << "array: [" << std::endl;
        const auto& array = datum.value<std::vector<avro::GenericDatum>>();
        for (const auto& elem : array) {
            printGenericDatum(elem, indent + 1);
        }
        std::cout << indentSpaces(indent) << "]" << std::endl;
        break;
    }

    case avro::AVRO_MAP: {
        std::cout << "map: {" << std::endl;
        const auto& map = datum.value<std::map<std::string, avro::GenericDatum>>();
        for (const auto& [key, value] : map) {
            std::cout << indentSpaces(indent + 1) << key << ": ";
            printGenericDatum(value, indent + 1);
        }
        std::cout << indentSpaces(indent) << "}" << std::endl;
        break;
    }

    case avro::AVRO_RECORD: {
        std::cout << "record: {" << std::endl;
        try {
            const auto& record = datum.value<avro::GenericRecord>();
            for (size_t i = 0; i < record.schema()->leaves(); ++i) {
                const auto& field = record.fieldAt(i);
                const auto& fieldName = record.schema()->nameAt(i);
                std::cout << indentSpaces(indent + 1) << fieldName << ": ";
                printGenericDatum(field, indent + 1);
            }
            std::cout << indentSpaces(indent) << "}" << std::endl;
            break;
        }
        catch (const std::bad_any_cast& e) {
            spdlog::error("std::any_cast fallito: {}", e.what());
        }

    }

    case avro::AVRO_ENUM: {
        const auto& enumValue = datum.value<avro::GenericEnum>();
        std::cout << "enum: " << enumValue.symbol() << std::endl;
        break;
    }

    case avro::AVRO_FIXED: {
        const auto& fixed = datum.value<avro::GenericFixed>().value();
        std::cout << "fixed: ";
        for (uint8_t byte : fixed) {
            std::cout << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
        break;
    }

    default:
        std::cout << "unknown type" << std::endl;
        break;
    }
}
*/

////////////////////////////////////////////
std::vector<uint8_t> Worker1::processData(const std::vector<uint8_t>& data, int priority) {
    std::vector<uint8_t> binary_result;    
    // result["dataType"] = "binary";

    std::cout << "Dentro Worker1::processData " << std::endl;
    std::cout << "Received data size: " << data.size() << std::endl;    // Size of the binary buffer

    std::string dataflow_type = get_supervisor()->dataflowtype;

    if (dataflow_type == "binary") {

        std::cout << "Dentro Worker1::processData con DATAFLOWTYPE == BINARY" << std::endl;
        std::cout << "\n RICEZIONE DI Worker1::processData():" << std::endl;

        // Verifica dimensione minima
        if (data.size() < sizeof(int32_t)) {
            std::cerr << "Error: Received data size is smaller than expected." << std::endl;
            return binary_result; // Restituisci un vettore vuoto
        }

        // Estrai la dimensione del payload
        int32_t size;
        std::memcpy(&size, data.data(), sizeof(int32_t));  // Read the size from the buffer

        if (size <= 0 || size > static_cast<int32_t>(data.size() - sizeof(int32_t))) {
            std::cerr << "Invalid size value: " << size << std::endl;
            return binary_result;
        }

        const HeaderWF* receivedPacket = reinterpret_cast<const HeaderWF*>(data.data());
        // std::memcpy(&receivedPacket, vec.data(), sizeof(HeaderWF));
        // std::cout << "Ci sono4" << std::endl;

        // Verify the content of the debufferized data
        std::cout << "Debufferized Header APID: " << receivedPacket->h.apid << std::endl;
        std::cout << "Debufferized Data size: " << receivedPacket->d.size << std::endl;
        std::cout << "Size of timespec: " << sizeof(receivedPacket->h.ts) << ", Alignment:" << alignof(receivedPacket->h.ts) << "\n" << std::endl;

        HeaderWF::print(*receivedPacket, 10);

        // result["data"] = data;
        // Simulate processing
        //std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(random_duration())));

        // Copia il payload in un vettore separato
        std::vector<uint8_t> vec(size);
        std::memcpy(vec.data(), data.data() + sizeof(int32_t), size);

        binary_result.insert(binary_result.end(), data.begin(), data.end());
        // binary_result.insert(binary_result.end(), vec.begin(), vec.end());  // Ritorna il payload del pacchetto per caricarlo sulla coda
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
    
    std::cout << "Esco da Worker1::processData " << std::endl;

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
