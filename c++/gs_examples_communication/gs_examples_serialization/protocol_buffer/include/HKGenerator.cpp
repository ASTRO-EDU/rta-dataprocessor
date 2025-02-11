#include <iostream>

#include "HKGenerator.hh"  // Include your generated header file for protobuf messages
#include "TimeSpecGen.hh"

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
    // Create an instance of TimeSpec

    c1.set_abstime(static_cast<int32_t>(epoch_time));
    c1.set_apid(rand() % 100);
    c1.set_configid(123);
    c1.set_counter(pc++);
    c1.set_crc(rand());
    c1.set_flags(rand());
    c1.set_runid(runID);
    c1.set_allocated_time(generate_time());
    c1.set_wformcount(rand());
    c1.set_type(1);
}
