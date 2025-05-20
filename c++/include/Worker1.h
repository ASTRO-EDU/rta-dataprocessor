#ifndef WORKER1_H
#define WORKER1_H

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
#include <time.h>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include "Supervisor.h"
#include "tensorflow/lite/c/c_api.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"


class Worker1 : public WorkerBase {
private:
    // Inference time tracking
    static std::atomic<int> global_inference_count;
    static std::atomic<double> global_total_time;
    static std::mutex global_stats_mutex;
    const int REPORT_INTERVAL = 10000; // Report average after every 10000 inferences

    // Helper function to generate random duration between 0 and 100 milliseconds
    double random_duration();

    TfLiteInterpreter* interp_ = nullptr;
    TfLiteTensor* input_tensor_ = nullptr;
    const TfLiteTensor* output_tensor_ = nullptr;

    TfLiteInterpreter* loadInterpreter(const std::string& model_path);

    double timespec_diff(const struct timespec* start, const struct timespec* end);

public:
    // Constructor
    Worker1();

    // Destructor
    ~Worker1() override;

    // Override the config method
    void config(const nlohmann::json& configuration);

    // void printGenericDatum(const avro::GenericDatum& datum, int indent);

    // Override the process_data method
    // std::string process_data(const std::string& data);
    std::vector<uint8_t> processData(const std::vector<uint8_t>& data, int priority);
};

#endif // WORKER1_H
