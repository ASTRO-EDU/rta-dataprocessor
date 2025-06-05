#ifndef WORKERMANAGER2_H
#define WORKERMANAGER2_H

#include "WorkerManager.h"
#include "Worker2.h"
#include "WorkerThread.h"
#include "WorkerProcess.h"
#include <memory>
#include <vector>
#include <string>

class WorkerManager2 : public WorkerManager, public std::enable_shared_from_this<WorkerManager2> {
public:
    // Constructor
    WorkerManager2(int manager_id, Supervisor* supervisor, const std::string& name = "");

    // Override to start worker threads
    void start_worker_threads(int num_threads);

    // Override to start worker processes
    // void start_worker_processes(int num_processes);

private:
    int manager_id;
};

#endif // WORKERMANAGER2_H