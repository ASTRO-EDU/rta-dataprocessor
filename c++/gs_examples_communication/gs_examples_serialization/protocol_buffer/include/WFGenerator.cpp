#include <iostream>
#include <random>
#include <vector>

#include "WFGenerator.hh"
#include "TimeSpecGen.hh"

#define WF_SIZE 16384
const int32_t min_value = 0;
const int32_t max_value = 16384; // 2^14
// Constructor definition

WFGenerator::WFGenerator(const int runID) : runID(runID), pc(0) {}

// Method to get the HeaderHK object
HeaderandWaveform WFGenerator::get() {
    generate_data();
    return c1;
}

// Private method to generate data
void WFGenerator::generate_data() {
    c1.clear_data();
    auto now = std::chrono::system_clock::now();
    std::time_t epoch_time = std::chrono::system_clock::to_time_t(now);   
    // Create an instance of TimeSpec
    TimeSpec* time_spec = new TimeSpec();
    time_spec->set_tv_sec(1625156761); // Example value for tv_sec (UNIX timestamp)
    time_spec->set_tv_nsec(500000000); // Example value for tv_nsec (nanoseconds)


    c1.set_abstime(static_cast<int32_t>(epoch_time));
    c1.set_apid(rand() % 100);
    c1.set_configid(123);
    c1.set_counter(pc++);
    c1.set_crc(rand());
    c1.set_runid(runID);
    c1.set_allocated_time(time_spec);
    c1.set_type(1);
    c1.set_equalizationlevel(rand() % 10);
    c1.set_decimation(rand() % 10000);
    c1.set_curoffset(rand() % 10000);
    c1.set_trigoff(rand() % 10000);
    c1.set_size(WF_SIZE);

    // Create a random number generator
    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_int_distribution<int32_t> dis(min_value, max_value); // Define the range

    // Create and fill the vector with random values
    for (std::size_t i = 0; i < WF_SIZE; ++i) {
        c1.add_data(dis(gen));
    }

    c1.set_crc(rand());
}
