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
#include <cstring>

#include <H5Cpp.h>
#include <tinyxml2.h>

using namespace tinyxml2;

// constants for scaling
constexpr int kNumSamples = 1000;
constexpr float in_min = 74.0f;     // 0.0
constexpr float in_max = 3821.0f;   // 3500.0
constexpr float out_min = 3418.73f;
constexpr float out_max = 149944.45f;

std::atomic<int> Worker1::global_inference_count{ 0 };
std::atomic<double> Worker1::global_total_time{ 0.0 };
std::mutex Worker1::global_stats_mutex;
std::atomic<int> Worker1::peak_memory_kb{ 0 };

std::atomic<int> Worker1::batch_counter{ 0 };
std::vector<GFRow> Worker1::dl2_data;
DL2ModelDefinition Worker1::dl2_model;
std::mutex Worker1::init_mutex;
bool Worker1::model_initialized = false;


// Constructor
Worker1::Worker1() : WorkerBase() {
    const std::string model_file = "/home/gamma/float_16.tflite";
    interp_ = loadInterpreter(model_file);
    input_tensor_ = TfLiteInterpreterGetInputTensor(interp_, 0);
    output_tensor_ = TfLiteInterpreterGetOutputTensor(interp_, 0);

    // Create output directory if it doesn't exist
    const std::string output_dir = "/home/gamma/rtadp-c/c++/output";
    if (!std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
        // std::cout << "[Worker1] Created output directory: " << output_dir << std::endl;
    }

    // Thread-safe initialization of DL2 model
    std::lock_guard<std::mutex> lock(init_mutex);
    if (!model_initialized) {
        dl2_model = parseDL2Model("/home/gamma/DL2model.xml");
        model_initialized = true;
        std::cout << "[Worker1] DL2 model initialized with group '" << dl2_model.groupName << "' and dataset '" << dl2_model.datasetName << "'" << std::endl;
    }
}

Worker1::~Worker1() {
    if (interp_) TfLiteInterpreterDelete(interp_);
}

// Override the config method
void Worker1::config(const nlohmann::json& configuration) {
    WorkerBase::config(configuration);
}

DL2ModelDefinition Worker1::parseDL2Model(const std::string& xmlPath) {
    std::cout << "[Worker1] Parsing DL2 model from: " << xmlPath << std::endl;

    DL2ModelDefinition model;
    XMLDocument doc;

    if (doc.LoadFile(xmlPath.c_str()) != XML_SUCCESS) {
        throw std::runtime_error("Failed to load XML file: " + xmlPath);
    }

    XMLElement* rootElem = doc.FirstChildElement("dl2model");
    if (!rootElem) {
        throw std::runtime_error("Missing root element 'dl2model' in XML file");
    }

    XMLElement* groupElem = rootElem->FirstChildElement("group");
    if (!groupElem) {
        throw std::runtime_error("Missing 'group' element in XML file");
    }

    const char* groupName = groupElem->Attribute("name");
    if (!groupName) {
        throw std::runtime_error("Missing 'name' attribute in group element");
    }
    model.groupName = groupName;
    std::cout << "[Worker1] Found group name: '" << model.groupName << "'" << std::endl;

    XMLElement* datasetElem = groupElem->FirstChildElement("dataset");
    if (!datasetElem) {
        throw std::runtime_error("Missing 'dataset' element in XML file");
    }

    const char* datasetName = datasetElem->Attribute("name");
    if (!datasetName) {
        throw std::runtime_error("Missing 'name' attribute in dataset element");
    }
    model.datasetName = datasetName;
    std::cout << "[Worker1] Found dataset name: '" << model.datasetName << "'" << std::endl;

    XMLElement* fieldsElem = datasetElem->FirstChildElement("fields");
    if (!fieldsElem) {
        throw std::runtime_error("Missing 'fields' element in XML file");
    }

    XMLElement* fieldElem = fieldsElem->FirstChildElement("field");
    while (fieldElem) {
        const char* fieldName = fieldElem->Attribute("name");
        const char* fieldType = fieldElem->Attribute("type");

        if (!fieldName || !fieldType) {
            throw std::runtime_error("Field element missing required attributes");
        }

        FieldDefinition field;
        field.name = fieldName;
        field.type = fieldType;
        model.fields.push_back(field);
        std::cout << "[Worker1] Found field: '" << field.name << "' of type '" << field.type << "'" << std::endl;

        fieldElem = fieldElem->NextSiblingElement("field");
    }

    if (model.fields.empty()) {
        throw std::runtime_error("No fields found in XML file");
    }

    std::cout << "[Worker1] Successfully parsed DL2 model with " << model.fields.size() << " fields" << std::endl;
    return model;
}

void Worker1::write_dl2_file(const DL2ModelDefinition& model, const std::string& h5filename, const std::vector<GFRow>& data) {
    // Debug output
    // std::cout << "[Worker1] Creating HDF5 file: " << h5filename << std::endl;
    // std::cout << "[Worker1] Group name: '" << model.groupName << "'" << std::endl;
    // std::cout << "[Worker1] Dataset name: '" << model.datasetName << "'" << std::endl;

    if (model.groupName.empty()) {
        throw std::runtime_error("Group name cannot be empty");
    }
    if (model.datasetName.empty()) {
        throw std::runtime_error("Dataset name cannot be empty");
    }

    H5::H5File file(h5filename, H5F_ACC_TRUNC);
    H5::Group group = file.createGroup(model.groupName);

    H5::CompType mtype(sizeof(GFRow));
    for (const auto& field : model.fields) {
        std::cout << "[Worker1] Processing field: '" << field.name << "' of type '" << field.type << "'" << std::endl;

        if (field.name.empty()) {
            throw std::runtime_error("Field name cannot be empty");
        }

        if (field.name == "n_waveform")
            mtype.insertMember(field.name, HOFFSET(GFRow, n_waveform), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "mult")
            mtype.insertMember(field.name, HOFFSET(GFRow, mult), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "tstart")
            mtype.insertMember(field.name, HOFFSET(GFRow, tstart), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "index_peak")
            mtype.insertMember(field.name, HOFFSET(GFRow, index_peak), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "peak")
            mtype.insertMember(field.name, HOFFSET(GFRow, peak), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "integral1")
            mtype.insertMember(field.name, HOFFSET(GFRow, integral1), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "integral2")
            mtype.insertMember(field.name, HOFFSET(GFRow, integral2), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "integral3")
            mtype.insertMember(field.name, HOFFSET(GFRow, integral3), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "halflife")
            mtype.insertMember(field.name, HOFFSET(GFRow, halflife), H5::PredType::NATIVE_FLOAT);
        else if (field.name == "temp")
            mtype.insertMember(field.name, HOFFSET(GFRow, temp), H5::PredType::NATIVE_FLOAT);
        else
            throw std::runtime_error("Unknown field in XML: " + field.name);
    }

    hsize_t dims[1] = { data.size() };
    H5::DataSpace dataspace(1, dims);
    H5::DataSet dataset = group.createDataSet(model.datasetName, mtype, dataspace);

    dataset.write(data.data(), mtype);
    std::cout << "[Worker1] Successfully wrote " << data.size() << " rows to HDF5 file" << std::endl;
}


// Helper: load & prepare the interpreter
TfLiteInterpreter* Worker1::loadInterpreter(const std::string& model_path) {
    TfLiteModel* model = TfLiteModelCreateFromFile(model_path.c_str());
    if (!model) {
        std::cerr << "[Worker1] ERROR: cannot load model " << model_path << "\n";
        std::exit(1);
    }

    // XNNPack for acceleration
    TfLiteXNNPackDelegateOptions xnn_opts = TfLiteXNNPackDelegateOptionsDefault();
    TfLiteDelegate* xnnpack = TfLiteXNNPackDelegateCreate(&xnn_opts);

    TfLiteInterpreterOptions* opts = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsAddDelegate(opts, xnnpack);
    // TfLiteInterpreterOptionsSetNumThreads(opts, 6);
    // Litert uses per se a single thread to run inference. Notably even when telling the interpreter to use all 6 core the inference time doesn't change possibily due 
    // to the model being too small and the overheads dominate


    TfLiteInterpreter* interp = TfLiteInterpreterCreate(model, opts);
    TfLiteInterpreterOptionsDelete(opts);

    if (TfLiteInterpreterAllocateTensors(interp) != kTfLiteOk) {
        std::cerr << "[Worker1] ERROR: AllocateTensors failed\n";
        std::exit(1);
    }
    return interp;
}

double Worker1::timespec_diff(const struct timespec* start, const struct timespec* end) {
    time_t sec = end->tv_sec - start->tv_sec;
    long   nsec = end->tv_nsec - start->tv_nsec;
    if (nsec < 0) {
        --sec;
        nsec += 1'000'000'000L;
    }
    return double(sec) + double(nsec) / 1e9;
}

int Worker1::getMemoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
 
    return usage.ru_maxrss;  // Returns memory usage in kilobytes
}

////////////////////////////////////////////
std::vector<uint8_t> Worker1::processData(const std::vector<uint8_t>& data, int priority) {
    std::vector<uint8_t> binary_result;    
    std::string dataflow_type = get_supervisor()->dataflowtype;

    if (dataflow_type == "binary") {
        // Check minimum dimension
        if (data.size() < sizeof(int32_t)) {
            std::cerr << "[Worker1] Error: Received data size is smaller than expected." << std::endl;
            return binary_result; // Return an empty vector
        }

        // Extract the size of the packet (first 4 bytes)
        int32_t size;
        std::memcpy(&size, data.data(), sizeof(int32_t));  

        // Size has to be non-negative and does not exceed the available data in data
        if (size <= 0 || size > data.size() - sizeof(int32_t)) {
            std::cerr << "[Worker1] Invalid size value: " << size << std::endl;
        }

        std::vector<uint8_t> vec(size);
        vec.resize(size);    // Resize the data vector to hold the full payload

        // Store into vec only the actual packet data, excluding the size field
        //memcpy(vec.data(), static_cast<const uint8_t*>(data.data()), size);
        memcpy(vec.data(), data.data() + sizeof(int32_t), size);

        const uint8_t* raw_data = vec.data();

        // Cast the extracted data from the queue to WfPacketDams (gs_examples_communication/gs_examples_serialization/ccsds/include/packet.h)
        const WfPacketDams* packet = reinterpret_cast<const WfPacketDams*>(raw_data);
        uint8_t packet_type = packet->body.d.type;  // Store type in a variable

        // std::cout << "[Worker1] Packet type = 0x" << std::hex << int(packet_type) << std::dec << "\n";
        // const uint32_t* unaligned_buf = packet->body.d.buff;

        // Copy into aligned storage
        constexpr size_t N = 1000;   // Number of uint32_t words (waveform size)
        alignas(4) uint32_t aligned_buf[N];
        std::memcpy(aligned_buf, packet->body.d.buff, sizeof(aligned_buf));

        if (packet_type == Data_HkDams::TYPE) { // HK Packet
            std::cout << "[Worker1] Housekeeping packet received. Printing infos: " << std::endl;
        }
        else if (packet_type == Data_WaveData::TYPE) {
            // std::cout << "[Worker1] Waveform packet received. Starting inference.\n";

            // Allocate a float buffer of 2×Nwords samples (since every waveform is composed of 2 float 16 samples)
            std::vector<float> float_wave(2 * N);

            // Unpack each word into two uint16_t samples, then convert to float
            for (int i = 0; i < N; ++i) {
                uint32_t word = aligned_buf[i];
                uint16_t lo = word & 0xFFFF;
                uint16_t hi = (word >> 16) & 0xFFFF;

                // Cast from uint to float (since we use a fp16 model)
                float_wave[2 * i] = static_cast<float>(hi);
                float_wave[2 * i + 1] = static_cast<float>(lo);
            }

            int64_t raw_sum = 0;
            for (float x : float_wave) raw_sum += x;            
	        // std::cout << "[Worker1] Raw sum = " << raw_sum << "\n";

            // Diagnostic info to understand value distribution
            float min_val = *std::min_element(float_wave.begin(), float_wave.end());
            float max_val = *std::max_element(float_wave.begin(), float_wave.end());
            float avg_val = static_cast<float>(raw_sum) / float_wave.size();

            // Pre-scale into the input tensor using the MinMax Scaling used for the model definition
            float* model_in = reinterpret_cast<float*>(TfLiteTensorData(input_tensor_));
            for (int i = 0; i < 2 * kNumSamples; ++i) {
                float x = float_wave[i];
                x = std::clamp(x, in_min, in_max);
                model_in[i] = 2.f * (x - in_min) / (in_max - in_min) - 1.f;     // Forward MinMax Scaling to [-1, 1]
            }

            double inference_time;
            struct timespec start, end;

            clock_gettime(CLOCK_MONOTONIC_RAW, &start);

            // Run inference
            if (TfLiteInterpreterInvoke(interp_) != kTfLiteOk) {
                std::cerr << "[Worker1] ERROR: inference failed\n";
                return binary_result;
            }

            clock_gettime(CLOCK_MONOTONIC_RAW, &end);
            inference_time = timespec_diff(&start, &end);

            int current_memory = getMemoryUsage();
            int previous_peak = peak_memory_kb.load();

            while (current_memory > previous_peak) {
                if (peak_memory_kb.compare_exchange_weak(previous_peak, current_memory)) {
                    break;
                }
            }

            // Read raw prediction and inverse-scale
            const float* model_out = reinterpret_cast<const float*>(TfLiteTensorData(output_tensor_));
            float y_pred = model_out[0];
            float y_orig = (y_pred + 1.f) * 0.5f * (out_max - out_min) + out_min;       // Inverse MinMax Scaling from [-1, 1]
            
            // std::cout << "[Worker1] Predicted model output (inverse-scaled area): " << y_orig << "\n";
            // std::cout << "[Worker1] Predicted model output (scaled area ([-1, 1])): " << y_pred << "\n";
            // std::cout << "[Worker1] Output scaling details: min_area_value=" << out_min << ", max_area_value=" << out_max << ", scale=" << ((out_max - out_min) * 0.5f) << "\n";
            // std::cout << "[Worker1] Waveform value range: min=" << min_val << ", max=" << max_val << ", avg=" << avg_val << std::endl;
            // std::cout << "[Worker1] Total inference time: " << inference_time << "s\n";

            {
                std::lock_guard<std::mutex> lock(global_stats_mutex);
                global_total_time.store(global_total_time.load() + inference_time);
                global_inference_count++;


                // DL2 data collection
                GFRow row;
                row.n_waveform = dl2_data.size() + 1;  // 1-based index
                row.mult = -1.0;

                // const Data_WaveHeader* packet2 = reinterpret_cast<const Data_WaveHeader*>(raw_data);
                row.tstart = static_cast<float>(packet->body.w.ts.tv_sec);  // Convert nanoseconds to float
                
                row.index_peak = -1.0f;
                row.peak = -1.0;    // max_val could be used
                row.integral1 = y_orig;  
                row.integral2 = -1.0f;
                row.integral3 = -1.0f;
                row.halflife = -1.0f;
                row.temp = -1.0f;        

                dl2_data.push_back(row);

                // If we've collected 1000 waveforms, write to file
                if (dl2_data.size() >= REPORT_INTERVAL) {
                    std::string filename = "/home/gamma/rtadp-c/c++/output/dl2_output_batch" + std::to_string(batch_counter++) + ".dl2.h5";

                    try {
                        write_dl2_file(dl2_model, filename, dl2_data);
                        std::cout << "[Worker1] Wrote DL2 file: " << filename << " with " << dl2_data.size() << " rows\n";
                        dl2_data.clear();  // Clear the vector after writing
                    }
                    catch (const std::exception& e) {
                        std::cerr << "[Worker1] Error writing DL2 file: " << e.what() << "\n";
                    }
                }
                    

                if (global_inference_count >= REPORT_INTERVAL) {
                    double avg_time = global_total_time.load() / global_inference_count.load();

                    std::cout << "[Worker1] ===== INFERENCE STATISTICS =====\n";
                    std::cout << "[Worker1] Processed " << global_inference_count.load() << " waveforms\n";
                    std::cout << "[Worker1] Average inference time: " << avg_time << "s\n";
                    std::cout << "[Worker1] Inference rate: " << (1.0 / avg_time) << " Hz\n";
                    std::cout << "[Worker1] Peak memory usage: " << peak_memory_kb.load() << " KB\n";
                    std::cout << "[Worker1] ================================\n";

                    // Reset counters
                    global_total_time.store(0.0);
                    global_inference_count.store(0);
                    peak_memory_kb.store(0);
                }
            }

            // Pack the final un-scaled area into the return vector
            binary_result.resize(sizeof(float));
            std::memcpy(binary_result.data(), &y_orig, sizeof(float));

            return binary_result;
        }
        else {
            std::cerr << "[Worker1] Unknown packet type: " << packet_type << std::endl;
        }

        binary_result.insert(binary_result.end(), vec.begin(), vec.end());  // Append data at the end
    } 
    else if (dataflow_type == "filename") {
        /*
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
        */
    }
    else if (dataflow_type == "string") {
        /*
        nlohmann::json result;

        const std::string str_data(data.begin(), data.end());
        std::cout << "\nProcessed string data: " << str_data << std::endl;

        result["data"] = str_data;

        std::string current_time = get_current_time_as_string();
        result["timestamp"] = current_time;

        std::string json_str = result.dump();
        binary_result = std::vector<uint8_t>(json_str.begin(), json_str.end());

        std::cout << "binary_result: " << binary_result.size() << std::endl;
        */
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
