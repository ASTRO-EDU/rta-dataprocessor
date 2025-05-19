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

// constants for scaling
constexpr int kNumSamples = 1000;
constexpr float in_min = 74.0f;     // 0.0
constexpr float in_max = 3821.0f;   // 3500.0
constexpr float out_min = 3418.73f;
constexpr float out_max = 149944.45f;


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

    TfLiteInterpreter* interp = TfLiteInterpreterCreate(model, opts);
    TfLiteInterpreterOptionsDelete(opts);

    if (TfLiteInterpreterAllocateTensors(interp) != kTfLiteOk) {
        std::cerr << "[Worker1] ERROR: AllocateTensors failed\n";
        std::exit(1);
    }
    return interp;
}

// Constructor
Worker1::Worker1() : WorkerBase() {
    // adjust path as needed
    const std::string model_file = "/home/gamma/float_16.tflite";
    interp_ = loadInterpreter(model_file);
    input_tensor_ = TfLiteInterpreterGetInputTensor(interp_, 0);
    output_tensor_ = TfLiteInterpreterGetOutputTensor(interp_, 0);
}

Worker1::~Worker1() {
    if (interp_) TfLiteInterpreterDelete(interp_);
}

// Override the config method
void Worker1::config(const nlohmann::json& configuration) {
    WorkerBase::config(configuration);
}

double Worker1::timespec_diff(struct timespec* start, struct timespec* end) {
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) / 1e9;
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

	    /*
        std::cout << "[Worker1] debug: start     = 0x"
            << std::hex << int(packet->body.h.start) << "\n"
            << "[Worker1] debug: w.type    = 0x"
            << int(packet->body.w.type) << "\n"
            << "[Worker1] debug: d.type    = 0x"
            << int(packet->body.d.type)
            << std::dec << "\n";
	    */

        uint8_t packet_type = packet->body.d.type;  // Store type in a variable

        // std::cout << "[Worker1] Packet type = 0x" << std::hex << int(packet_type) << std::dec << "\n";
        // const uint32_t* unaligned_buf = packet->body.d.buff;

        // Copy into aligned storage
        constexpr size_t N = 1000;   // Number of uint32_t words (waveform size)
        alignas(4) uint32_t aligned_buf[N];
        std::memcpy(aligned_buf, packet->body.d.buff, sizeof(aligned_buf));

        /*
        // Print them all
        std::cout << "[Worker1] Full waveform buffer (" << N << " words):\n    ";
        for (size_t i = 0; i < N; ++i) {
            // print as hex, padded to 8 digits
            std::cout << "0x"
                << std::hex << std::setw(8) << std::setfill('0')
                << aligned_buf[i]
                << " ";
                // newline every 8 words for readability
                if ((i + 1) % 8 == 0 && i + 1 < N)
                    std::cout << "\n    ";
        }
        std::cout << std::dec << std::setfill(' ') << "\n";
        */


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

	        /*
            // Print a sanity check:
            std::cout << "[Worker1] Samples (100-400):";
            for (int i = 100; i < 500; ++i) {
                std::cout << " " << float_wave[i];
            }
            std::cout << std::endl;
	        */

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

	        /*
            // Print some scaled input values
            std::cout << "[Worker1] Scaled input samples (0-10):";
            for (int i = 0; i < 10; ++i) {
                std::cout << " " << model_in[i];
            }
            std::cout << std::endl;
	        */

            float inference_time;
            struct timespec start, end;

            clock_gettime(CLOCK_MONOTONIC, &start);

            // Run inference
            if (TfLiteInterpreterInvoke(interp_) != kTfLiteOk) {
                std::cerr << "[Worker1] ERROR: inference failed\n";
                return binary_result;
            }

            clock_gettime(CLOCK_MONOTONIC, &end);
            inference_time= timespec_diff(&start, &end);

            // Read raw prediction and inverse-scale
            const float* model_out = reinterpret_cast<const float*>(TfLiteTensorData(output_tensor_));
            float y_pred = model_out[0];
            float y_orig = (y_pred + 1.f) * 0.5f * (out_max - out_min) + out_min;       // Inverse MinMax Scaling from[-1, 1]
            
            std::cout << "[Worker1] Predicted model output (inverse-scaled area): " << y_orig << "\n";
            std::cout << "[Worker1] Predicted model output (scaled area ([-1, 1])): " << y_pred << "\n";
            std::cout << "[Worker1] Output scaling details: min_area_value=" << out_min << ", max_area_value=" << out_max << ", scale=" << ((out_max - out_min) * 0.5f) << "\n";
            std::cout << "[Worker1] Waveform value range: min=" << min_val << ", max=" << max_val << ", avg=" << avg_val << std::endl;

            std::cout << "[Worker1] Total inference time: " << inference_time << std::endl;


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
