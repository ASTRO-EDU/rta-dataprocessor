#include <iostream>
#include <random>
#include <vector>

#include "WFGenerator.hh"


const int32_t min_value = 0;
const int32_t max_value = 16384; // 2^14
// Constructor definition
WFGenerator::WFGenerator(const int runID) : runID(runID), pc(0) {}

// Method to get the HeaderHK object
WF::HeaderandWaveform WFGenerator::get() {
    generate_data();
    return c1;
}

// Private method to generate data
void WFGenerator::generate_data() {
    auto now = std::chrono::system_clock::now();
    std::time_t epoch_time = std::chrono::system_clock::to_time_t(now);
    int32_t epoch_time_32 = static_cast<int32_t>(epoch_time);
    
    clock_gettime(CLOCK_REALTIME, &t1);
    
    c1.apid = 30303;
    c1.counter = pc++;
    c1.type = 1;
    c1.time.tv_sec = t1.tv_sec;
    c1.time.tv_nsec = t1.tv_nsec;
    c1.abstime = epoch_time_32;
    c1.runID = runID;
    c1.configID = 123;

    c1.equalizationLevel = rand() % 10;
    c1.decimation = rand() % 10000;
    c1.curOffset = rand() % 10000;
    c1.trigOff = rand() % 10000;
    c1.size = WF_SIZE;
    c1.data.resize(WF_SIZE);

    // Create a random number generator
    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_int_distribution<int32_t> dis(min_value, max_value); // Define the range

    // Create and fill the vector with random values
    for (std::size_t i = 0; i < WF_SIZE; ++i) {
        c1.data[i] = dis(gen);
    }


    c1.crc = rand();
}
