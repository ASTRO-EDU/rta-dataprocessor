#ifndef __GENERATORS_HH__
#define __GENERATORS_HH__

#include <cstring>
#include <iostream>
#include <random>
#include <chrono>
#include "packet.h"

const int min_value = 0;
const int max_value = 16384;

class HKGenerator {
public:
    // Constructor that initializes the runID
    HKGenerator(const int runID);

    // Method to generate and return HeaderHK object
    HeaderHK get();

private:
    // Private member variables
    SerializedTimespec t1;           // For storing time
    HeaderHK c1;        // The HeaderHK object
    int runID;             // Identifier for the run
    int pc;                // Counter for the HeaderHK object

    // Private method to generate data
    void generate_data();
};

class WFGenerator {
public:

    // Constructor that initializes the runID
    WFGenerator(const int runID);

    // Method to generate and return HeaderHK object
    HeaderWF get();

private:
    // Private member variables
    SerializedTimespec t1;           // For storing time
    HeaderWF c1;        // The HeaderHK object
    int runID;             // Identifier for the run
    int pc;                // Counter for the HeaderHK object

    // Private method to generate data
    void generate_data();
};


#endif