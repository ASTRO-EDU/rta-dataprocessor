#ifndef PB_WF_GENERATOR_HH
#define PB_WF_GENERATOR_HH

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib> // For rand()
#include <ctime>   // For clock_gettime()

#include "WF.pb.h" // Replace with the actual header file that defines c::HeaderHK

class WFGenerator {
public:

    // Constructor that initializes the runID
    WFGenerator(const int runID);

    // Method to generate and return HeaderHK object
    HeaderandWaveform get();

private:
    // Private member variables
    HeaderandWaveform c1;        // The HeaderHK object
    int runID;             // Identifier for the run
    int pc;                // Counter for the HeaderHK object

    // Private method to generate data
    void generate_data();
};

#endif // PB_WF_GENERATOR_HH