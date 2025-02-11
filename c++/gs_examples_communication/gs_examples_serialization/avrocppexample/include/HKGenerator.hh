#ifndef HK_GENERATOR_HH
#define HK_GENERATOR_HH

#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib> // For rand()
#include <ctime>   // For clock_gettime()

#include "HKSchema.hh" // Replace with the actual header file that defines c::HeaderHK

class HKGenerator {
public:
    // Constructor that initializes the runID
    HKGenerator(const int runID);

    // Method to generate and return HeaderHK object
    HK::HeaderHK get();

private:
    // Private member variables
    timespec t1;           // For storing time
    HK::HeaderHK c1;        // The HeaderHK object
    int runID;             // Identifier for the run
    int pc;                // Counter for the HeaderHK object

    // Private method to generate data
    void generate_data();
};

#endif // HK_GENERATOR_HH