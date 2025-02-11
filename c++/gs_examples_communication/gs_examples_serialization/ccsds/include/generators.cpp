#include <cstring>
#include <iostream>
#include <random>
#include <chrono>

#include "packet.h"
#include "generators.hh"

// Constructor definition
HKGenerator::HKGenerator(const int runID) : runID(runID), pc(0) {}

// Method to get the HeaderHK object
HeaderHK HKGenerator::get() {
    generate_data();
    return c1;
}

// Private method to generate data
void HKGenerator::generate_data() {
    auto now = std::chrono::system_clock::now();
    std::time_t epoch_time = std::chrono::system_clock::to_time_t(now);
    int32_t epoch_time_32 = static_cast<int32_t>(epoch_time);
    static int pc = 0;

    c1.h.apid     = 3000;
    c1.h.counter  = pc++;
    c1.h.type     = 20;
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    c1.h.ts = serializeTimespec(ts);
    c1.h.abstime  = epoch_time_32;
    c1.h.runID    = 1;
    c1.h.configID = 130;

    c1.d.wformcount = rand() % 5000;
    c1.d.flags      = 0xABCD;
    c1.d.crc        = 0xEF01;

}

WFGenerator::WFGenerator(const int runID) : runID(runID), pc(0) {}

// Method to get the HeaderHK object
HeaderWF WFGenerator::get() {
    generate_data();
    return c1;
}

// Private method to generate data
void WFGenerator::generate_data() {

    auto now = std::chrono::system_clock::now();
    std::time_t epoch_time = std::chrono::system_clock::to_time_t(now);
    int32_t epoch_time_32 = static_cast<int32_t>(epoch_time);
     
    
    c1.h.apid = 30303;
    c1.h.counter = pc++;
    c1.h.type = 1;
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    c1.h.ts = serializeTimespec(ts);
    c1.h.abstime = epoch_time_32;
    c1.h.runID = 209;
    c1.h.configID = 123;

   

    c1.d.equalizationLevel = rand() % 10;
    c1.d.decimation = rand() % 10000;
    c1.d.curOffset = rand() % 10000;
    c1.d.trigOff = rand() % 10000;
    c1.d.size = WF_SIZE;

    // Create a random number generator
    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_int_distribution<int32_t> dis(min_value, max_value); // Define the range

    // Create and fill the vector with random values
    for (std::size_t i = 0; i < WF_SIZE; ++i) {
        c1.d.data[i] = dis(gen);
    }

    c1.d.crc = rand();

}
