// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>
#include <json/json.h>  // Assuming a suitable JSON library
#include <sys/sysinfo.h>  // For system resource monitoring

class MonitoringPoint {
public:
    MonitoringPoint(WorkerManager* manager)
        : manager(manager), processOS(getpid()) {
        data["header"]["type"] = 1;
        data["header"]["time"] = 0;  // Replace with actual timestamp if needed
        data["header"]["pidsource"] = manager->fullname;
        data["header"]["pidtarget"] = "*";

        data["workermanagerstatus"] = "Initialised";  // Append the "status" key to the data dictionary
        data["procinfo"]["cpu_percent"] = 0;
        data["procinfo"]["memory_usage"] = 0;
        data["queue_lp_size"] = 0;
        data["queue_hp_size"] = 0;

        std::cout << "MonitoringPoint initialised" << std::endl;
    }

    // Update a specific key-value pair in the data dictionary
    void update(const std::string& key, const Json::Value& value) {
        std::lock_guard<std::mutex> lock(data_mutex);
        data[key] = value;
    }

    // Retrieve the current data with updated system resource monitoring
    Json::Value get_data() {
        std::lock_guard<std::mutex> lock(data_mutex);
        resource_monitor();
        data["header"]["time"] = std::time(0);
        set_status(manager->status);
        data["stopdatainput"] = manager->stopdata;
        update("queue_lp_size", manager->low_priority_queue->size());
        update("queue_hp_size", manager->high_priority_queue->size());
        update("queue_lp_result_size", manager->result_lp_queue->size());
        update("queue_hp_result_size", manager->result_hp_queue->size());
        update("workersstatusinit", manager->workersstatusinit);
        update("workersstatus", manager->workersstatus);
        update("workersname", manager->workersname);

        if (manager->processingtype == "process") {
            for (const auto& worker : manager->worker_processes) {
                processing_rates[worker->worker_id] = manager->processing_rates_shared[worker->worker_id];
                processing_tot_events[worker->worker_id] = manager->total_processed_data_count_shared[worker->worker_id];
                worker_status[worker->worker_id] = manager->worker_status_shared[worker->worker_id];
            }
        } else if (manager->processingtype == "thread") {
            for (const auto& worker : manager->worker_threads) {
                processing_rates[worker->worker_id] = worker->processing_rate;
                processing_tot_events[worker->worker_id] = worker->total_processed_data_count;
                worker_status[worker->worker_id] = worker->status;
            }
        }

        data["worker_rates"] = processing_rates;
        data["worker_tot_events"] = processing_tot_events;
        data["worker_status"] = worker_status;
        return data;
    }

    // Set a new status for the worker manager
    void set_status(const std::string& new_status) {
        std::lock_guard<std::mutex> lock(data_mutex);
        data["workermanagerstatus"] = new_status;
    }

    // Retrieve the current status of the worker manager
    std::string get_status() {
        std::lock_guard<std::mutex> lock(data_mutex);
        return data["workermanagerstatus"].asString();
    }

private:
    WorkerManager* manager;
    pid_t processOS;
    Json::Value data;
    std::unordered_map<int, double> processing_rates;
    std::unordered_map<int, int> processing_tot_events;
    std::unordered_map<int, int> worker_status;
    std::mutex data_mutex;

    // Monitor system resources like CPU and memory usage
    void resource_monitor() {
        struct sysinfo memInfo;
        sysinfo(&memInfo);

        data["procinfo"]["cpu_percent"] = get_cpu_usage();
        data["procinfo"]["memory_usage"] = memInfo.totalram - memInfo.freeram;
    }

    // Function to get CPU usage percentage
    double get_cpu_usage() {
        // Placeholder for actual CPU usage calculation
        // This function needs to be implemented with actual system calls to get CPU usage
        return 0.0;
    }
};

