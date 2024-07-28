// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <psutil.h>  // Assume a suitable library for psutil functions
#include "MonitoringPoint.h"
#include "WorkerThread.h"
#include "MonitoringThread.h"
#include "Supervisor.h"

class WorkerManager {
public:
    // Constructor
    WorkerManager(int manager_id, Supervisor* supervisor, const std::string& name = "None")
        : manager_id(manager_id), supervisor(supervisor), name(name), 
          status("Initialising"), continueall(true), processdata(0), stopdata(true), 
          _stop_event(false) {

        workersname = supervisor->name_workers[manager_id];
        config = supervisor->config;
        logger = supervisor->logger;
        fullname = supervisor->name + "-" + name;
        globalname = "WorkerManager-" + fullname;
        processingtype = supervisor->processingtype;
        max_workers = 100;
        result_socket_type = supervisor->manager_result_sockets_type[manager_id];
        result_lp_socket = supervisor->manager_result_lp_sockets[manager_id];
        result_hp_socket = supervisor->manager_result_hp_sockets[manager_id];
        result_dataflow_type = supervisor->manager_result_dataflow_type[manager_id];
        socket_lp_result = supervisor->socket_lp_result;
        socket_hp_result = supervisor->socket_hp_result;
        pid = psutil::Process::get_pid();
        context = supervisor->context;
        socket_monitoring = supervisor->socket_monitoring;

        if (processingtype == "thread") {
            low_priority_queue = std::make_shared<std::queue<std::string>>();
            high_priority_queue = std::make_shared<std::queue<std::string>>();
            result_lp_queue = std::make_shared<std::queue<std::string>>();
            result_hp_queue = std::make_shared<std::queue<std::string>>();
        }

        monitoringpoint = nullptr;
        monitoring_thread = nullptr;
        num_workers = 0;
        workersstatus = 0;
        workersstatusinit = 0;

        if (processingtype == "thread") {
            tokenresultslock = std::make_shared<std::mutex>();
            tokenreadinglock = std::make_shared<std::mutex>();
        }

        spdlog::info("{} started", globalname);
        logger->system("Started", globalname);
        spdlog::info("Socket result parameters: {} / {} / {} / {}", result_socket_type, result_lp_socket, result_hp_socket, result_dataflow_type);
        logger->system(fmt::format("Socket result parameters: {} / {} / {} / {}", result_socket_type, result_lp_socket, result_hp_socket, result_dataflow_type), globalname);

        status = "Initialised";
        supervisor->send_info(1, status, fullname, 1, "Low");
    }

    // Function to change token results
    void change_token_results() {
        if (processingtype == "thread") {
            std::lock_guard<std::mutex> lock(*tokenresultslock);
            for (auto& worker : worker_threads) {
                worker->tokenresult = worker->tokenresult - 1;
                if (worker->tokenresult < 0) {
                    worker->tokenresult = num_workers - 1;
                }
            }
        }
    }

    // Function to change token reading
    void change_token_reading() {
        if (processingtype == "thread") {
            std::lock_guard<std::mutex> lock(*tokenreadinglock);
            for (auto& worker : worker_threads) {
                worker->tokenreading = worker->tokenreading - 1;
                if (worker->tokenreading < 0) {
                    worker->tokenreading = num_workers - 1;
                }
            }
        }
    }

    // Function to set stop data flag
    void set_stopdata(bool stopdata) {
        this->stopdata = stopdata;
        change_status();
    }

    // Function to set process data flag
    void set_processdata(int processdata) {
        this->processdata = processdata;
        change_status();

        if (processingtype == "process") {
            processdata_shared = processdata;
        } else if (processingtype == "thread") {
            for (auto& worker : worker_threads) {
                worker->set_processdata(this->processdata);
            }
        }
    }

    // Function to change the status based on flags
    void change_status() {
        if (stopdata && processdata == 0) {
            status = "Initialised";
        } else if (stopdata && processdata == 1) {
            status = "Wait for data";
        } else if (!stopdata && processdata == 1) {
            status = "Processing";
        } else if (!stopdata && processdata == 0) {
            status = "Wait for processing";
        }
        supervisor->send_info(1, status, fullname, 1, "Low");
    }

    // Function to start service threads
    void start_service_threads() {
        monitoringpoint = new MonitoringPoint(this);
        monitoring_thread = new std::thread(&MonitoringThread::run, new MonitoringThread(socket_monitoring, monitoringpoint));
    }

    // Function to start worker threads (to be reimplemented)
    virtual void start_worker_threads(int num_threads) {
        if (num_threads > max_workers) {
            spdlog::warn("WARNING! It is not possible to create more than {} threads", max_workers);
            logger->warning(fmt::format("WARNING! It is not possible to create more than {} threads", max_workers), globalname);
        }
        num_workers = num_threads;
    }

    // Function to start worker processes (to be reimplemented)
    virtual void start_worker_processes(int num_processes) {
        if (num_processes > max_workers) {
            spdlog::warn("WARNING! It is not possible to create more than {} processes", max_workers);
            logger->warning(fmt::format("WARNING! It is not possible to create more than {} processes", max_workers), globalname);
        }
        num_workers = num_processes;
    }

    // Main run function
    void run() {
        start_service_threads();

        status = "Initialised";
        supervisor->send_info(1, status, fullname, 1, "Low");

        try {
            while (!continueall) {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // To avoid 100% CPU consumption

                // Check the status of the workers
                workersstatus = 0;
                workersstatusinit = 0;
                int worker_id = 0;

                for (auto& process : worker_processes) {
                    int status = worker_status_shared[worker_id];
                    if (status == 0) {
                        workersstatusinit++;
                    } else {
                        workersstatus += status;
                    }
                    worker_id++;
                }

                for (auto& thread : worker_threads) {
                    if (thread->status == 0) {
                        workersstatusinit++;
                    } else {
                        workersstatus += thread->status;
                    }
                }

                if (num_workers != workersstatusinit) {
                    workersstatus = workersstatus / (num_workers - workersstatusinit);
                }
            }

            spdlog::info("Manager stop {}", globalname);
            logger->system("Manager stop", globalname);
        } catch (const std::exception& e) {
            spdlog::error("Exception caught: {}", e.what());
            logger->system(fmt::format("Exception caught: {}", e.what()), globalname);
            stop_internalthreads();
            continueall = false;
        }
    }

    // Function to clean the queues
    void clean_queue() {
        spdlog::info("Cleaning queues...");
        logger->system("Cleaning queues...", globalname);

        clean_single_queue(low_priority_queue, "low_priority_queue");
        clean_single_queue(high_priority_queue, "high_priority_queue");
        clean_single_queue(result_lp_queue, "result_lp_queue");
        clean_single_queue(result_hp_queue, "result_hp_queue");

        spdlog::info("End cleaning queues");
        logger->system("End cleaning queues", globalname);
    }

    // Function to stop the manager
    void stop(bool fast = false) {
        if (processingtype == "process") {
            if (!fast) {
                spdlog::info("Closing queues...");
                logger->system("Closing queues...", globalname);
                close_queue(low_priority_queue, "low_priority_queue");
                close_queue(high_priority_queue, "high_priority_queue");
                close_queue(result_lp_queue, "result_lp_queue");
                close_queue(result_hp_queue, "result_hp_queue");
                spdlog::info("End closing queues");
                logger->system("End closing queues", globalname);
            }
        }

        // Stop worker threads
        for (auto& thread : worker_threads) {
            thread->stop();
            thread->join();
        }
        _stop_event = true;
        stop_internalthreads();
        status = "End";
    }

    // Function to stop internal threads
    void stop_internalthreads() {
        spdlog::info("Stopping Manager internal threads...");
        logger->system("Stopping Manager internal threads...", globalname);
        monitoring_thread->detach();
        delete monitoring_thread;
        spdlog::info("All Manager internal threads terminated.");
        logger->system("All Manager internal threads terminated.", globalname);
    }

    // Function to configure workers
    void configworkers(const Json::Value& configuration) {
        if (processingtype == "thread") {
            for (auto& worker : worker_threads) {
                worker->config(configuration);
            }
        }
    }

private:
    int manager_id;
    Supervisor* supervisor;
    std::string name;
    std::string status;
    std::string workersname;
    std::shared_ptr<Json::Value> config;
    WorkerLogger* logger;
    std::string fullname;
    std::string globalname;
    std::string processingtype;
    int max_workers;
    std::string result_socket_type;
    std::string result_lp_socket;
    std::string result_hp_socket;
    std::string result_dataflow_type;
    std::vector<zmq::socket_t*> socket_lp_result;
    std::vector<zmq::socket_t*> socket_hp_result;
    int pid;
    zmq::context_t* context;
    zmq::socket_t* socket_monitoring;
    std::shared_ptr<std::queue<std::string>> low_priority_queue;
    std::shared_ptr<std::queue<std::string>> high_priority_queue;
    std::shared_ptr<std::queue<std::string>> result_lp_queue;
    std::shared_ptr<std::queue<std::string>> result_hp_queue;
    MonitoringPoint* monitoringpoint;
    std::thread* monitoring_thread;
    std::vector<WorkerThread*> worker_threads;
    std::vector<WorkerThread*> worker_processes;
    int num_workers;
    int workersstatus;
    int workersstatusinit;
    std::shared_ptr<std::mutex> tokenresultslock;
    std::shared_ptr<std::mutex> tokenreadinglock;
    std::atomic<bool> continueall;
    std::atomic<int> processdata;
    std::atomic<bool> stopdata;
    std::atomic<bool> _stop_event;
    std::atomic<int> processdata_shared;
    std::vector<std::atomic<int>> worker_status_shared;

    // Helper function to clean a single queue
    void clean_single_queue(std::shared_ptr<std::queue<std::string>>& queue, const std::string& queue_name) {
        if (!queue->empty()) {
            spdlog::info("   - {} size {}", queue_name, queue->size());
            logger->system(fmt::format("   - {} size {}", queue_name, queue->size()), globalname);
            while (!queue->empty()) {
                queue->pop();
            }
            spdlog::info("   - {} empty", queue_name);
            logger->system(fmt::format("   - {} empty", queue_name), globalname);
        }
    }

    // Helper function to close a queue
    void close_queue(std::shared_ptr<std::queue<std::string>>& queue, const std::string& queue_name) {
        try {
            spdlog::info("   - {} size {}", queue_name, queue->size());
            logger->system(fmt::format("   - {} size {}", queue_name, queue->size()), globalname);
            while (!queue->empty()) {
                queue->pop();
            }
            queue.reset();
            spdlog::info("   - {} empty", queue_name);
            logger->system(fmt::format("   - {} empty", queue_name), globalname);
        } catch (const std::exception& e) {
            spdlog::error("ERROR in worker stop {} cleaning: {}", queue_name, e.what());
            logger->error(fmt::format("ERROR in worker stop {} cleaning: {}", queue_name, e.what()), globalname);
        }
    }
};

