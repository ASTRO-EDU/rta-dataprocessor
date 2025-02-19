#include "Supervisor2.h"

// Constructor
Supervisor2::Supervisor2(const std::string& config_file, const std::string& name)
    : Supervisor(config_file, name) {
}

// Destructor
Supervisor2::~Supervisor2() {
    for (WorkerManager* m: manager_workers)
        delete m;
}

// Override the start_managers method
void Supervisor2::start_managers() {
    int indexmanager = 0;
    WorkerManager* manager2 = new WorkerManager2(indexmanager, this, workername);
    setup_result_channel(manager2, indexmanager);
    manager2->run();
    manager_workers.push_back(manager2);
    logger->info("DER SUP2 manager started");
}

// For "dataflowtype": "binary", decode the data before loading it into the queue.
zmq::message_t& Supervisor2::decode_data(zmq::message_t& data) {
    return data;
}

// For "dataflowtype": "file", open the file before loading it into the queue. 
// Return an array of data and the size of the array
std::pair<std::vector<std::string>, int> Supervisor2::open_file(const std::string& filename) {
    std::vector<std::string> f = {filename};
    return {f, 1};
}
