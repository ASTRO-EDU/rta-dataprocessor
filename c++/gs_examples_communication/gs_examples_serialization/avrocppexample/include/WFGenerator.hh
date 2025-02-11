#ifndef WF_GENERATOR_HH
#define WF_GENERATOR_HH

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib> // For rand()
#include <ctime>   // For clock_gettime()

#include "WFSchema.hh" // Replace with the actual header file that defines c::HeaderHK

#define WF_SIZE 16384

class WFGenerator {
public:

    // Constructor that initializes the runID
    WFGenerator(const int runID);

    // Method to generate and return HeaderHK object
    WF::HeaderandWaveform get();

private:
    // Private member variables
    timespec t1;           // For storing time
    WF::HeaderandWaveform c1;        // The HeaderHK object
    int runID;             // Identifier for the run
    int pc;                // Counter for the HeaderHK object

    // Private method to generate data
    void generate_data();
};

#endif // WF_GENERATOR_HH