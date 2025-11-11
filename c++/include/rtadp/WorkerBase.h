#ifndef WORKERBASE_H
#define WORKERBASE_H

#include <string>
#include <iostream>
#include <rtadp/json.hpp> 
#include <zmq.hpp>     
#include <rtadp/WorkerLogger.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/fmt/fmt.h"


class Supervisor;
class WorkerManager;

class WorkerBase {
    Supervisor* supervisor = nullptr;
    WorkerManager* manager = nullptr;
    std::string fullname;

public:
    std::string workersname;
    WorkerLogger* logger;

    WorkerBase();
    virtual ~WorkerBase();

    // Initialize the worker with manager, supervisor, and names
    void init(WorkerManager* manager, Supervisor* supervisor, const std::string& workersname, const std::string& fullname);

    virtual void config(const nlohmann::json& configuration);

    // virtual std::string 
    // const std::string& data);
    virtual std::vector<uint8_t> processData(const std::vector<uint8_t>& data, int priority) = 0;

    Supervisor* get_supervisor() const{{
        return supervisor;
    }}


};

#endif // WORKERBASE_H
