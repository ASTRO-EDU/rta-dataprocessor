#include <iostream>
#include "HKGenerator.hh"

// Constructor definition
HKGenerator::HKGenerator(const int runID) : runID(runID), pc(0) {}

// Method to get the HeaderHK object
HK::HeaderHK HKGenerator::get() {
    generate_data();
    return c1;
}

// Private method to generate data
void HKGenerator::generate_data() {
    auto now = std::chrono::system_clock::now();
    std::time_t epoch_time = std::chrono::system_clock::to_time_t(now);
    int32_t epoch_time_32 = static_cast<int32_t>(epoch_time);
    
    clock_gettime(CLOCK_REALTIME, &t1);
    
    c1.abstime = epoch_time_32;
    c1.apid = rand() % 100;
    c1.configID = 123;
    c1.counter = pc++;
    c1.crc = rand();
    c1.flags = rand();
    c1.runID = runID;
    c1.time.tv_sec = t1.tv_sec;
    c1.time.tv_nsec = t1.tv_nsec;
    c1.wformcount = rand();
    c1.type = 1;

}
