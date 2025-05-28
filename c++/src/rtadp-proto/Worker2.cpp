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
    std::cout << "Inside Worker2::processData" << std::endl;
    std::cout << "Worker2::process_data priority:" << priority << std::endl;

    return {};
}
