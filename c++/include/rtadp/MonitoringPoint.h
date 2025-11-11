#ifndef MONITORINGPOINT_H
#define MONITORINGPOINT_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>
#include <rtadp/json.hpp>  
#include <sys/sysinfo.h>  
#include <rtadp/WorkerLogger.h>


class Supervisor;
class WorkerManager;

class MonitoringPoint {
    Supervisor* supervisor;
    WorkerManager* manager;  // Pointer to the WorkerManager
    pid_t processOS;  // Process ID of the current process
    nlohmann::json data;  // JSON object to store data
    std::unordered_map<int, double> processing_rates;  // Map of processing rates
    std::unordered_map<int, int> processing_tot_events;  // Map of total processing events
    std::unordered_map<int, int> worker_status;  // Map of worker statuses
    std::mutex data_mutex;  // Mutex for thread-safe access to data
    WorkerLogger* logger;

    // Monitors and updates system resources (CPU, memory)
    void resource_monitor();

    // Placeholder function to get CPU usage
    double get_cpu_usage();

public:
    // Constructor to initialize the MonitoringPoint with a WorkerManager pointer
    MonitoringPoint(WorkerManager* manager);

    // Updates the data map with a new key-value pair
    void update(const std::string& key, const nlohmann::json& value);

    // Retrieves the current data including system resource monitoring
    nlohmann::json get_data();

    // Sets the status in the data map
    void set_status(const std::string& new_status);

    // Gets the current status from the data map
    std::string get_status();

};

#endif // MONITORINGPOINT_H
