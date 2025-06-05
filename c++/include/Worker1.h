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
#include <sys/resource.h>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include "Supervisor.h"
#include "tensorflow/lite/c/c_api.h"
#include "tensorflow/lite/delegates/xnnpack/xnnpack_delegate.h"


// Struct definitions for DL2 XML file handling
struct FieldDefinition {
    std::string name;
    std::string type;  
};

// Struct that is used as a type
struct DL2ModelDefinition {
    std::string groupName;
    std::string datasetName;
    std::vector<FieldDefinition> fields;
};

// Structure used to group data that will be written on the DL2 file
struct GFRow {
    float n_waveform;
    float mult;
    float tstart;
    float index_peak;
    float peak;
    float integral1;
    float integral2;
    float integral3;
    float halflife;
    float temp;
};

class Worker1 : public WorkerBase {
private:
    // Static members for tracking shared statistics during inference across all workers
    static std::atomic<int> global_inference_count;
    static std::atomic<double> global_total_time;
    static std::mutex global_stats_mutex;
    const int REPORT_INTERVAL = 1000; // Report average after every 10000 inferences
    static std::atomic<int> peak_memory_kb;

    // Static members for shared DL2 file writing across all workers
    static std::atomic<int> batch_counter;
    static std::vector<GFRow> dl2_data;
    static DL2ModelDefinition dl2_model;
    static std::mutex init_mutex;
    static bool model_initialized;

    // Path helper methods
    static std::string getModelPath();
    static std::string getOutputPath();
    static int getDL2Rows();

    TfLiteInterpreter* interp_ = nullptr;
    TfLiteTensor* input_tensor_ = nullptr;
    const TfLiteTensor* output_tensor_ = nullptr;

    TfLiteInterpreter* loadInterpreter(const std::string& model_path);

    double timespec_diff(const struct timespec* start, const struct timespec* end);

    int getMemoryUsage();  // Returns memory usage in KB

    // DL2 file handling functions
    static DL2ModelDefinition parseDL2Model(const std::string& xmlPath);
    static void write_dl2_file(const DL2ModelDefinition& model, const std::string& h5filename, const std::vector<GFRow>& data);


public:
    // Constructor
    Worker1();

    // Destructor
    ~Worker1() override;

    // Override the config method
    void config(const nlohmann::json& configuration);

    // Override the process_data method
    std::vector<uint8_t> processData(const std::vector<uint8_t>& data, int priority);
};

#endif // WORKER1_H
