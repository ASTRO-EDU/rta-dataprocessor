// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <iostream>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include "Supervisor.h"
#include "WorkerManager.h"
#include "Worker.h"

class WorkerThread {
public:
    WorkerThread(int worker_id, WorkerManager* manager, const std::string& name, Worker* worker)
        : worker_id(worker_id), manager(manager), name(name), worker(worker), 
          processdata(0), status(0), tokenresult(worker_id), tokenreading(worker_id), _stop_event(false) {

        supervisor = manager->supervisor;
        workersname = supervisor->name + "-" + manager->name + "-" + name;
        fullname = workersname + "-" + std::to_string(worker_id);
        globalname = "WorkerThread-" + fullname;
        logger = supervisor->logger;

        worker->init(manager, supervisor, workersname, fullname);

        low_priority_queue = manager->low_priority_queue;
        high_priority_queue = manager->high_priority_queue;
        monitoringpoint = manager->monitoringpoint;

        start_time = std::chrono::high_resolution_clock::now();
        next_time = start_time;
        processed_data_count = 0;
        total_processed_data_count = 0;
        processing_rate = 0.0;

        spdlog::info("{} started", globalname);
        logger->system("WorkerThread started", globalname);
    }

    // Function to stop the worker thread
    void stop() {
        status = 16; // stop
        _stop_event = true;
    }

    // Function to configure the worker thread
    void config(const Json::Value& configuration) {
        worker->config(configuration);
    }

    // Function to set process data flag
    void set_processdata(int processdata1) {
        processdata = processdata1;
    }

    // Main run function
    void run() {
        start_timer(1);

        while (!_stop_event) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(10)); // must be 0

            if (processdata == 1 && tokenreading == 0) {
                try {
                    // Check and process high-priority queue first
                    if (!high_priority_queue->empty()) {
                        auto high_priority_data = high_priority_queue->front();
                        high_priority_queue->pop();
                        manager->change_token_reading();
                        process_data(high_priority_data, 1);
                    } else {
                        // Process low-priority queue if high-priority queue is empty
                        if (!low_priority_queue->empty()) {
                            auto low_priority_data = low_priority_queue->front();
                            low_priority_queue->pop();
                            manager->change_token_reading();
                            process_data(low_priority_data, 0);
                        } else {
                            status = 2; // waiting for new data
                        }
                    }
                } catch (const std::exception& e) {
                    spdlog::error("Exception caught in WorkerThread run: {}", e.what());
                }
            } else {
                if (tokenreading != 0 && status != 4) {
                    status = 4; // waiting for reading from queue
                }
            }
        }

        timer->detach();
        spdlog::info("WorkerThread stop {}", globalname);
        logger->system("WorkerThread stop", globalname);
    }

private:
    int worker_id;
    WorkerManager* manager;
    Supervisor* supervisor;
    Worker* worker;
    std::string name;
    std::string workersname;
    std::string fullname;
    std::string globalname;
    WorkerLogger* logger;
    std::shared_ptr<std::queue<std::string>> low_priority_queue;
    std::shared_ptr<std::queue<std::string>> high_priority_queue;
    MonitoringPoint* monitoringpoint;

    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
    std::chrono::time_point<std::chrono::high_resolution_clock> next_time;
    int processed_data_count;
    int total_processed_data_count;
    double processing_rate;
    std::atomic<bool> _stop_event;
    std::atomic<int> processdata;
    std::atomic<int> status;
    int tokenresult;
    int tokenreading;
    std::unique_ptr<std::thread> timer;

    // Function to start a timer
    void start_timer(int interval) {
        timer = std::make_unique<std::thread>(&WorkerThread::workerop, this, interval);
    }

    // Worker operation function
    void workerop(int interval) {
        while (!_stop_event) {
            std::this_thread::sleep_for(std::chrono::seconds(interval));

            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - next_time).count();
            next_time = std::chrono::high_resolution_clock::now();
            processing_rate = static_cast<double>(processed_data_count) / elapsed_time;
            total_processed_data_count += processed_data_count;
            spdlog::info("{} Rate Hz {:.1f} Current events {} Total events {} Queues {} {}", globalname, processing_rate, processed_data_count, total_processed_data_count, low_priority_queue->size(), high_priority_queue->size());
            logger->system(fmt::format("Rate Hz {:.1f} Current events {} Total events {} Queues {} {}", processing_rate, processed_data_count, total_processed_data_count, low_priority_queue->size(), high_priority_queue->size()), globalname);
            processed_data_count = 0;
        }
    }

    // Function to process data
    void process_data(const std::string& data, int priority) {
        status = 8; // processing new data
        processed_data_count++;

        auto dataresult = worker->process_data(data);

        if (dataresult && tokenresult == 0) {
            if (priority == 0) {
                manager->result_lp_queue->push(dataresult);
            } else {
                manager->result_hp_queue->push(dataresult);
            }
            manager->change_token_results();
        }
    }
};

