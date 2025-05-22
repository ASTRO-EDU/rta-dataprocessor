#ifndef WORKERMANAGER1_H
#define WORKERMANAGER1_H

#include "WorkerManager.h"
#include "Worker1.h"
#include "WorkerThread.h"
#include "WorkerProcess.h"
#include <memory>
#include <vector>
#include <string>

class WorkerManager1 : public WorkerManager, public std::enable_shared_from_this<WorkerManager1> {
public:
    // Constructor
    WorkerManager1(int manager_id, Supervisor* supervisor, const std::string& name = "");

    // Override to start worker threads
    void start_worker_threads(int num_threads) override;

    // Override to start worker processes
    // void start_worker_processes(int num_processes) override;

private:
    int manager_id;
};

#endif // WORKERMANAGER1_H
