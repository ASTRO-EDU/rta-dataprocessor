// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <memory>

#include <execinfo.h>
#include <unistd.h>

#include "ccsds/include/packet.h"
#include <iostream>

#include "WorkerThread.h"


using json = nlohmann::json;

//////////////////////////////////////////////////
WorkerThread::WorkerThread(int worker_id, WorkerManager* manager, const std::string& name, WorkerBase* worker)
    : worker_id(worker_id), manager(manager), name(name), worker(worker),
     processdata(0), status(0), tokenresult(worker_id), tokenreading(worker_id), _stop_event(false) {

    // TODO: Rimuovere print o meglio trasformare come log
    std::cout << "Creating a WorkerThread with name: " << name << std::endl;

    supervisor = manager->getSupervisor();
    workersname = supervisor->name + "-" + manager->getName() + "-" + name;
    fullname = workersname + "-" + std::to_string(worker_id);
    globalname = "WorkerThread-" + fullname;
    logger = supervisor->logger;

    worker->init(manager, supervisor, workersname, fullname);

    low_priority_queue = manager->getLowPriorityQueue();
    high_priority_queue = manager->getHighPriorityQueue();
    monitoringpoint = manager->getMonitoringPoint();

    start_time = std::chrono::high_resolution_clock::now();
    next_time = start_time;
    processed_data_count = 0;
    total_processed_data_count = 0;
    processing_rate = 0.0;

    logger->info("WorkerThread started", globalname);

    internal_thread = std::make_unique<std::thread>(&WorkerThread::run, this);
}
//////////////////////////////////////////////////

void WorkerThread::config(const json& configuration) {
    worker->config(configuration);
}

void WorkerThread::set_processdata(int processdata1) {
    processdata = processdata1;
}

//////////////////////////////////////////////////
void WorkerThread::run() {
    start_timer(1);

    while (!_stop_event) {
        // std::this_thread::sleep_for(std::chrono::nanoseconds(10));
        if (processdata == 1 && tokenreading == 0) {
                // Check and process high-priority queue first
                if (!high_priority_queue->empty()) {
                    auto high_priority_data = high_priority_queue->get();
                    manager->change_token_reading();
                    process_data(high_priority_data, 1);
                } 
                else {
                    // Process low-priority queue if high-priority queue is empty
                    if (!low_priority_queue->empty()) {
                        std::cout << "DENTRO WorkerThread::run" << std::endl;

                        auto low_priority_data = low_priority_queue->get();
                        manager->change_token_reading();

                        if (!low_priority_data.empty()) {
                            std::cout << "WorkerThread::run: ENTRO IN WorkerThread::process_data" << std::endl;
                            process_data(low_priority_data, 0);
                        }
                        else {
                            logger->error("low_priority_data data is empty!");
                        }
                    } 
                    else {
                        status = 2; // waiting for new data
                    }
                }

        } 
        else {
            if (tokenreading != 0 && status != 4) {
                status = 4; // waiting for reading from queue
            }
        }
    }

}

//////////////////////////////////////////////////
// Destructor
WorkerThread::~WorkerThread(){
    // Proteggi l'accesso a `worker`
    {
        std::lock_guard<std::mutex> lock(stop_worker_mutex);
        if (worker) {
            delete worker;
            worker = nullptr; 
        }
    }

    if (!_stop_event) {
        stop();
    }

}

//////////////////////////////////////////////////
void WorkerThread::stop() {
    if (_stop_event) {
        return;
    }

    _stop_event = true;

    // Notify all threads that are waiting on the queues
    low_priority_queue->notify_all();
    high_priority_queue->notify_all();

    if (internal_thread && internal_thread->joinable()) {
        internal_thread->join();
    }

    if (timer->joinable()) {
        timer->join();
    }

    status = 16; // Thread is terminated
}
//////////////////////////////////////////////////

int WorkerThread::get_tokenresult() const {
    return tokenresult;
}

void WorkerThread::set_tokenresult(int value) {
    tokenresult = value;
}

int WorkerThread::get_tokenreading() const {
    return tokenreading;
}

void WorkerThread::set_tokenreading(int value) {
    tokenreading = value;
}

int WorkerThread::get_status() const { 
    return status;
}

int WorkerThread::getWorkerId() const {
    return worker_id;
}

double WorkerThread::getProcessingRate() const {
    return processing_rate;
}

int WorkerThread::getTotalProcessedDataCount() const {
    return total_processed_data_count;
}

void WorkerThread::set_status(int value) { 
    status = value;
}

bool WorkerThread::joinable() const {
    return internal_thread && internal_thread->joinable();
}

void WorkerThread::join() {
    if (internal_thread && internal_thread->joinable()) {
        internal_thread->join();
    }
}

// Function to start a timer
void WorkerThread::start_timer(int interval) {
    timer = std::make_unique<std::thread>(&WorkerThread::workerop, this, interval);
}

void WorkerThread::workerop(int interval) {
    while (!_stop_event) {
        std::this_thread::sleep_for(std::chrono::seconds(interval));

        auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - next_time).count();
        next_time = std::chrono::high_resolution_clock::now();
        processing_rate = static_cast<double>(processed_data_count) / elapsed_time;
        total_processed_data_count += processed_data_count;
        // logger->warning(fmt::format("{} Rate Hz {:.1f} Current events {} Total events {} Queues {} {}", globalname, processing_rate, processed_data_count, total_processed_data_count, low_priority_queue->size(), high_priority_queue->size()));
        processed_data_count = 0;
    }
}

////////////////////////////////////////////
void WorkerThread::process_data(const std::vector<uint8_t>& data, int priority) {
    status = 8; // processing new data
    processed_data_count++;

    if (!worker) {
        return;
    }

    std::cout << "DENTRO WorkerThread::process_data  " << std::endl;
    auto dataresult = worker->processData(data, priority);
    std::cout << "TORNATO IN WorkerThread::process_data  " << std::endl;
    
    std::cout << "\n RICEZIONE DI WorkerThread::process_data:" << std::endl;


    // Verifica dimensione minima
    if (data.size() < sizeof(int32_t)) {
        std::cerr << "Error: Received data size is smaller than expected." << std::endl;
    }

    // Estrai la dimensione del payload
    int32_t size;
    std::memcpy(&size, data.data(), sizeof(int32_t));  // Read the size from the buffer

    if (size <= 0 || size > static_cast<int32_t>(data.size() - sizeof(int32_t))) {
        std::cerr << "Invalid size value: " << size << std::endl;
    }

    const HeaderWF* receivedPacket = reinterpret_cast<const HeaderWF*>(data.data());
    // std::memcpy(&receivedPacket, vec.data(), sizeof(HeaderWF));
    // std::cout << "Ci sono4" << std::endl;

    // Verify the content of the debufferized data
    std::cout << "Debufferized Header APID: " << receivedPacket->h.apid << std::endl;
    std::cout << "Debufferized Data size: " << receivedPacket->d.size << std::endl;
    std::cout << "Size of timespec: " << sizeof(receivedPacket->h.ts) << ", Alignment:" << alignof(receivedPacket->h.ts) << "\n" << std::endl;

    HeaderWF::print(*receivedPacket, 10);



    std::cout << "WorkerThread::process_data: DIMENSIONE: " << dataresult.size() << std::endl;

    if (!dataresult.empty() && tokenresult == 0) {
        std::cout << "WorkerThread::process_data: pusho sulla coda" << std::endl;

        if (priority == 0) {
            manager->getResultLpQueue()->push(dataresult);
        } 
        else {
            manager->getResultHpQueue()->push(dataresult);
        }
        manager->change_token_results();
    }
    else {
        std::cout << "WorkerThread::process_data: dataresult EMPTY" << std::endl;
    }
}
////////////////////////////////////////////