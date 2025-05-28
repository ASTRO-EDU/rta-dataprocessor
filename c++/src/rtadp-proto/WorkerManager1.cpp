#include "WorkerManager1.h"

// Constructor
WorkerManager1::WorkerManager1(int manager_id, Supervisor* supervisor, const std::string& name)
    : WorkerManager(manager_id, supervisor, name), manager_id(manager_id) {}

// Override to start worker threads
void WorkerManager1::start_worker_threads(int num_threads) {
    logger->info("Number of worker threads: ", std::to_string(num_threads));
    
    // Creating worker threads
    for (int i = 0; i < num_threads; ++i) {
        // auto processor = std::make_shared<Worker1>();
        WorkerBase* processor = new Worker1();

        const auto name_workers = getSupervisor()->name_workers;

        if (manager_id >= name_workers.size()) {
            logger->error(fmt::format("manager_id {} is out of bounds for name_workers size {}", manager_id, name_workers.size()));
            continue;  // Skip thread creation
        }

        // Check worker name
        std::string worker_name = name_workers[manager_id];
        if (worker_name.empty()) {
            logger->error("Worker name is empty for manager_id {} ", std::to_string(manager_id));
            continue;  // Skip thread creation
        }
        else {
            logger->info("A WorkerThread has been created for manager_id: ", std::to_string(manager_id));
        }

        auto worker_instance = std::make_shared<WorkerThread>(i, this, worker_name, processor);
        worker_threads.push_back(worker_instance);
    }
}

/*
// Override to start worker threads
void WorkerManager1::start_worker_threads(int num_threads) {
    // WorkerManager::start_worker_threads(num_threads);
    // Create worker threads
    for (int i = 0; i < num_threads; ++i) {
        auto processor = std::make_shared<Worker1>();
        auto thread = std::make_shared<WorkerThread>(i, this, getSupervisor()->getNameWorkers()[manager_id], processor.get());
        worker_threads.push_back(thread);
        thread->run();  // Start the thread
    }
}


// Override to start worker processes
void WorkerManager1::start_worker_processes(int num_processes) {
    WorkerManager::start_worker_processes(num_processes);
    // Create worker processes
    for (int i = 0; i < num_processes; ++i) {
        auto processor = std::make_shared<Worker1>();
        auto process = std::make_shared<WorkerProcess>(i, shared_from_this(), getSupervisor()->getNameWorkers()[manager_id], processor);
        getWorker_Processes().push_back(process);
        process->run();  // Start the process
    }
}
*/