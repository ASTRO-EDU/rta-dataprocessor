// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//

#include <string>
#include <iostream>
#include <json/json.h> // JSON library
#include <zmq.hpp>     // ZeroMQ library
#include "Logger.h"    // Assume you have a Logger class similar to Python's Logger
#include "Supervisor.h"
#include "WorkerManager.h"

// Base class for workers
class WorkerBase {
public:
    WorkerBase() = default;

    // Initialize the worker with manager, supervisor, and names
    void init(WorkerManager* manager, Supervisor* supervisor, const std::string& workersname, const std::string& fullname) {
        this->manager = manager;
        this->supervisor = supervisor;
        this->logger = supervisor->getLogger(); // Assuming Supervisor has a method getLogger()
        this->workersname = workersname;
        this->fullname = fullname;
    }

    // To be reimplemented in derived classes
    virtual void config(const Json::Value& configuration) {
        // Extract the pidtarget
        std::string pidtarget = configuration["header"]["pidtarget"].asString();

        // Check if the configuration is meant for this worker
        if (pidtarget == workersname || pidtarget == fullname) {
            std::cout << "Received config: " << configuration << std::endl;
        } else {
            return;
        }
    }

    // To be reimplemented in derived classes
    virtual void process_data(const std::string& data, int priority) {
        // Process data implementation to be provided in derived classes
    }

protected:
    WorkerManager* manager = nullptr;
    Supervisor* supervisor = nullptr;
    Logger* logger = nullptr; // Assuming Logger is a custom class similar to Python's Logger
    std::string workersname;
    std::string fullname;
};

