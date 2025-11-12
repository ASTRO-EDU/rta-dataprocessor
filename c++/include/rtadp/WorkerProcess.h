#ifndef WORKERPROCESS_H
#define WORKERPROCESS_H

#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <iostream>
#include <memory>
#include <unistd.h> // for pid_t and sleep
// #include <psutil.h> // Assuming you have a similar library for process management
#include <rtadp/WorkerManager.h> // Include the Manager class
#include <rtadp/WorkerBase.h> // Include the Worker class
#include <rtadp/json.hpp>  // Include nlohmann::json for configuration
#include <rtadp/WorkerLogger.h>

#include <rtadp/ThreadSafeQueue.h>

class WorkerProcess {
public:
    WorkerProcess(int worker_id, std::shared_ptr<WorkerManager> manager, const std::string& name, std::shared_ptr<WorkerBase> worker);
    ~WorkerProcess();

    void stop();
    void config(const nlohmann::json& configuration);  // Use nlohmann::json for configuration
    void run();
    void start_timer(int interval);
    
private:
    void workerop();
    void process_data(const nlohmann::json& data, int priority);

    int worker_id;
    std::shared_ptr<WorkerManager> manager;
    std::shared_ptr<Supervisor> supervisor;
    std::shared_ptr<WorkerBase> worker;

    //////////////////////
    std::shared_ptr<ThreadSafeQueue<std::vector<uint8_t>>> low_priority_queue;
    std::shared_ptr<ThreadSafeQueue<std::vector<uint8_t>>> high_priority_queue;
    /////////////////////

    std::string name;
    std::string workersname;
    std::string fullname;
    std::string globalname;

    pid_t pidprocess;

    WorkerLogger* logger;

    MonitoringPoint* monitoringpoint;

    std::atomic<bool> stop_event;

    double start_time;
    double next_time;
    int processed_data_count;
    int total_processed_data_count;
    double processing_rate;

    std::unique_ptr<std::thread> timer_thread;
};

#endif // WORKERPROCESS_H
