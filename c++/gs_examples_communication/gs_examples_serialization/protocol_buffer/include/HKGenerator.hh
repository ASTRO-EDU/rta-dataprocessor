#ifndef PB_HK_GENERATOR_HH
#define PB_HK_GENERATOR_HH

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib> // For rand()
#include <ctime>   // For clock_gettime()

#include "HK.pb.h"  // Include your generated header file for protobuf messages

class HKGenerator {
public:
    // Constructor that initializes the runID
    HKGenerator(const int runID);

    // Method to generate and return HeaderHK object
    HeaderHK get();

private:
    // Private member variables
    HeaderHK c1;        // The HeaderHK object
    int runID;             // Identifier for the run
    int pc;                // Counter for the HeaderHK object

    // Private method to generate data
    void generate_data();
};

#endif // PB_HK_GENERATOR_HH