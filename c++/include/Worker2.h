#ifndef WORKER2_H
#define WORKER2_H

#include "WorkerBase.h"
#include "avro/Generic.hh"
#include "avro/Schema.hh"
#include "avro/ValidSchema.hh"
#include "avro/Compiler.hh"
#include "avro/GenericDatum.hh"
#include "avro/DataFile.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "Supervisor.h"

class Worker2 : public WorkerBase {
private:

public:
    // Constructor
    Worker2();

    // Override the config method
    void config(const nlohmann::json& configuration);

    // Override the process_data method
    std::vector<uint8_t> processData(const std::vector<uint8_t>& data, int priority);
};

#endif // WORKER2_H