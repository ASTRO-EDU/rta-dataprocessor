// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//

#include "Supervisor.h"
#include "avro/Generic.hh"
#include "avro/Schema.hh"
#include "avro/ValidSchema.hh"
#include "avro/Compiler.hh"
#include "avro/GenericDatum.hh"
#include "avro/DataFile.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"
#include "ccsds/include/packet.h"
#include "../include/utils2.hh"


Supervisor::Supervisor(std::string config_file, std::string name)
    : name(name), continueall(true), config_manager(nullptr), manager_num_workers(0) {
    load_configuration(config_file, name);
    fullname = name;
    globalname = "Supervisor-" + name;

    // Set up logging
    std::string log_file = config["logs_path"].get<std::string>() + "/" + globalname + ".log";
    std::string logging_mode = config["logging"].get<std::string>();
    logger = new WorkerLogger("worker_logger", log_file, spdlog::level::trace, logging_mode);     // Level is set but is not used since we use the macro defined in CMakeList

    pid = getpid();
    context = zmq::context_t(1);

    try {
        int timeout = 300;    // 1000

        // Retrieve and log configuration
        processingtype = config["processing_type"].get<std::string>();
        dataflowtype = config["dataflow_type"].get<std::string>();
        logger->info("dataflowtype:", dataflowtype);
        datasockettype = config["datasocket_type"].get<std::string>();

        logger->info("Supervisor: " + globalname + " / " + dataflowtype + " / " 
                       + processingtype + " / " + datasockettype, globalname);

        // Set up data sockets based on configuration
        if (datasockettype == "pushpull") {
            socket_lp_data = new zmq::socket_t(context, ZMQ_PULL);
            socket_lp_data->bind(config["data_lp_socket"].get<std::string>());
            socket_lp_data->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
            socket_hp_data = new zmq::socket_t(context, ZMQ_PULL);
            socket_hp_data->bind(config["data_hp_socket"].get<std::string>());
        }
        else if (datasockettype == "pubsub") {
            socket_lp_data = new zmq::socket_t(context, ZMQ_SUB);
            socket_lp_data->connect(config["data_lp_socket"].get<std::string>());
            socket_lp_data->setsockopt(ZMQ_SUBSCRIBE, "", 0);
            socket_lp_data->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

            socket_hp_data = new zmq::socket_t(context, ZMQ_SUB);
            socket_hp_data->connect(config["data_hp_socket"].get<std::string>());
            socket_hp_data->setsockopt(ZMQ_SUBSCRIBE, "", 0);
            socket_hp_data->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        }
        else if (datasockettype == "custom") {
            logger->info("Supervisor started with custom data receiver", globalname);
        }
        else {
            throw std::invalid_argument("Config file: datasockettype must be pushpull or pubsub");
        }

        // Set up command and monitoring sockets
        socket_command = new zmq::socket_t(context, ZMQ_SUB);
        socket_command->connect(config["command_socket"].get<std::string>());
        socket_command->setsockopt(ZMQ_SUBSCRIBE, "", 0);
        socket_command->setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

        socket_monitoring = new zmq::socket_t(context, ZMQ_PUSH);
        socket_monitoring->connect(config["monitoring_socket"].get<std::string>());

        socket_lp_result.resize(100, nullptr);
        socket_hp_result.resize(100, nullptr);

    }
    catch (const std::exception& e) {
        // Handle any other unexpected exceptions
        std::cerr << "[Supervisor] ERROR: An unexpected error occurred: " << e.what() << std::endl;
        logger->warning("[Supervisor] ERROR: An unexpected error occurred: " + std::string(e.what()), globalname);
        exit(1);
    }

    manager_workers = std::vector<WorkerManager*>();
    processdata = 0;
    // stopdata = true;

    sendresultslock = std::make_shared<std::mutex>();

    status = "Initialised";
    send_info(1, status, fullname, 1, "Low");

    logger->info(globalname + " started", globalname);
}

// Destructor to clean up resources
Supervisor::~Supervisor() {
    std::cout << "[Supervisor] Cleaning up Supervisor resources..." << std::endl;
    if (lp_data_thread.joinable()) {
        lp_data_thread.join();
    }

    if (hp_data_thread.joinable()) {
        hp_data_thread.join();
    }

    if (result_thread.joinable()) {
        result_thread.join();
    }

    if (socket_command) {
        try {
            socket_command->close();
        }
        catch (const zmq::error_t& e) {
            logger->error("Error while closing socket_command: {}", e.what());
        }
        delete socket_command;
        socket_command = nullptr;
    }
    if (socket_lp_data) {
        try {
            socket_lp_data->close();
        }
        catch (const zmq::error_t& e) {
            logger->error("Error while closing socket_lp_data: {}", e.what());
        }
        delete socket_lp_data;
        socket_lp_data = nullptr;
    }
    if (socket_hp_data) {
        try {
            socket_hp_data->close();
        }
        catch (const zmq::error_t& e) {
            logger->error("Error while closing socket_hp_data: {}", e.what());
        }
        delete socket_hp_data;
        socket_hp_data = nullptr;
    }
    if (!socket_lp_result.empty()) {
        for (auto* socket : socket_lp_result) {
            if (socket) {
                try {
                    socket->close();
                }
                catch (const zmq::error_t& e) {
                    logger->error("Error while closing socket_lp_result: {}", e.what());
                }
                delete socket;
                socket = nullptr;
            }
        }
        socket_lp_result.clear();
    }
    if (!socket_hp_result.empty()) {
        for (auto* socket : socket_hp_result) {
            if (socket) {
                try {
                    socket->close();
                }
                catch (const zmq::error_t& e) {
                    logger->error("Error while closing socket_hp_result: {}", e.what());
                }
                delete socket;
                socket = nullptr;
            }
        }
        socket_hp_result.clear();
    }
    if (socket_monitoring) {
        try {
            socket_monitoring->close();
        }
        catch (const zmq::error_t& e) {
            logger->error("Error while closing socket_monitoring: {}", e.what());
        }
        delete socket_monitoring;
        socket_monitoring = nullptr;
    }


    zmq_ctx_shutdown(context.handle());

    try {
        context.close();
    }
    catch (const zmq::error_t& e) {
        logger->error("Error while closing ZMQ context: {}", e.what());
    }

    if (logger) {
        delete logger;
        logger = nullptr;
    }
}

static Supervisor* signal_supervisor_instance = nullptr;
// Static method to set the current instance
// void Supervisor::set_instance(Supervisor* instance) {
//     Supervisor::instance = instance;
// }

// // Static method to get the current instance
// Supervisor* Supervisor::get_instance() {
//     return Supervisor::instance;
// }

std::vector<std::string> Supervisor::getNameWorkers() const {
    return worker_names;
}

// Load configuration from the specified file and name
void Supervisor::load_configuration(const std::string& config_file, const std::string& name) {
    config_manager = new ConfigurationManager(config_file);
    config = config_manager->get_configuration(name);
    std::cout << config << std::endl;

    // Extract values from the tuple returned by get_workers_config
    auto workers_config = config_manager->get_workers_config(name);
    manager_result_sockets_type = std::get<0>(workers_config)[0];
    manager_result_dataflow_type = std::get<1>(workers_config)[0];
    manager_result_lp_sockets = std::get<2>(workers_config);
    manager_result_hp_sockets = std::get<3>(workers_config);
    manager_num_workers = std::get<4>(workers_config)[0]; // assuming single value
    workername = std::get<5>(workers_config)[0]; // assuming single value
    name_workers = std::get<6>(workers_config);
}

// Start service threads for data handling
void Supervisor::start_service_threads() {
    if (dataflowtype == "binary") {
        lp_data_thread = std::thread(&Supervisor::listen_for_lp_data, this);
        hp_data_thread = std::thread(&Supervisor::listen_for_hp_data, this);
    }
    else if (dataflowtype == "filename") {
        lp_data_thread = std::thread(&Supervisor::listen_for_lp_file, this);
        hp_data_thread = std::thread(&Supervisor::listen_for_hp_file, this);
    }
    else if (dataflowtype == "string") {
        lp_data_thread = std::thread(&Supervisor::listen_for_lp_string, this);
        hp_data_thread = std::thread(&Supervisor::listen_for_hp_string, this);
    }

    result_thread = std::thread(&Supervisor::listen_for_result, this);
}

// Set up result channel for a given WorkerManager
void Supervisor::setup_result_channel(WorkerManager* manager, int indexmanager) {
    socket_lp_result[indexmanager] = nullptr;
    socket_hp_result[indexmanager] = nullptr;
    //context = zmq::context_t(1);

    if (manager->get_result_lp_socket() != "none") {
        if (manager->get_result_socket_type() == "pushpull") {
            socket_lp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUSH);
            socket_lp_result[indexmanager]->connect(manager->get_result_lp_socket());
            logger->info("---result lp socket pushpull " + manager->get_globalname() + " " + manager->get_result_lp_socket(), globalname);
        }
        else if (manager->get_result_socket_type() == "pubsub") {
            socket_lp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUB);
            socket_lp_result[indexmanager]->bind(manager->get_result_lp_socket());
            logger->info("---result lp socket pubsub " + manager->get_globalname() + " " + manager->get_result_lp_socket(), globalname);
        }
        else {
            std::cerr << "Invalid socket type from config file." << std::endl;
            logger->error("Invalid socket type from config file.");
        }
    }

    if (manager->get_result_hp_socket() != "none") {
        if (manager->get_result_socket_type() == "pushpull") {
            socket_hp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUSH);
            socket_hp_result[indexmanager]->connect(manager->get_result_hp_socket());
            logger->info("---result hp socket pushpull " + manager->get_globalname() + " " + manager->get_result_hp_socket(), globalname);
        }
        else if (manager->get_result_socket_type() == "pubsub") {
            socket_hp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUB);
            socket_hp_result[indexmanager]->bind(manager->get_result_hp_socket());
            logger->info("---result hp socket pubsub " + manager->get_globalname() + " " + manager->get_result_hp_socket(), globalname);
        }
        else {
            std::cerr << "Invalid socket type from config file." << std::endl;
            logger->error("Invalid socket type from config file.");
        }
    }
}

// Start managers
void Supervisor::start_managers() {
    int indexmanager = 0;
    WorkerManager* manager = new WorkerManager(indexmanager, this, "Generic");
    setup_result_channel(manager, indexmanager);
    manager->run();
    manager_workers.push_back(manager);
    logger->info("[Supervisor] BASE SUP manager started.");
}

// Start workers
void Supervisor::start_workers() {
    int indexmanager = 0;

    for (auto& manager : manager_workers) {
        manager->start_worker_threads(manager_num_workers);
        logger->info("[Supervisor] BASE SUP start_worker_threads");
        indexmanager++;
    }
}

// Start Supervisor operations
void Supervisor::start() {
    logger->info("[Supervisor] Starting managers and workers");
    start_managers();
    start_workers();
    start_service_threads();

    status = "Waiting";
    send_info(1, status, fullname, 1, "Low");

    while (continueall) {
        listen_for_commands();
        std::this_thread::sleep_for(std::chrono::seconds(1)); // To avoid 100% CPU
    }
}


// // Handler C-style chiamato dal sistema
// void signal_handler(int signum) {
//     if (!Supervisor::get_instance()) return;

//     Supervisor::get_instance()->handle_signals(signum);
// }

void signal_handler(int signum) {
    if (!signal_supervisor_instance) return;

    signal_supervisor_instance->handle_signals(signum);
}

// Funzione da chiamare da main()
void setup_signal_handlers(Supervisor* supervisor) {
    signal_supervisor_instance = supervisor;

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) < 0) {
        std::cerr << "[ERROR] Failed to set SIGINT handler\n";
    }
    if (sigaction(SIGTERM, &sa, nullptr) < 0) {
        std::cerr << "[ERROR] Failed to set SIGTERM handler\n";
    }
}


//  function to handle signals
void Supervisor::handle_signals(int signum) {

    if (signum == SIGTERM) {
        std::cout << "\n[Supervisor] SIGTERM received in main thread. Terminating with cleaned shutdown." << std::endl;
        logger->warning("[Supervisor] SIGTERM received in main thread. Terminating with cleaned shutdown", signal_supervisor_instance->globalname);
        command_cleanedshutdown();
    }
    else if (signum == SIGINT) {
        std::cout << "\n[Supervisor] SIGINT received in main thread. Terminating with shutdown." << std::endl;
        logger->warning("[Supervisor] SIGINT received in main thread. Terminating with shutdown", signal_supervisor_instance->globalname);
        command_shutdown();
    }
    else {
        std::cout << "\n[Supervisor] Received signal " << signum << "in main thread. Terminating." << std::endl;
        logger->warning("[Supervisor] Received signal " + std::to_string(signum) + "in main thread. Terminating", signal_supervisor_instance->globalname);
        command_shutdown();
    }
}

// Listen for result data
void Supervisor::listen_for_result() {
    try {
        while (continueall) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 
            int indexmanager = 0;

            for (auto& manager : manager_workers) {
                int attempt = 0;  

                while (manager == nullptr && attempt < 10) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));  // Sleep for 1 second to avoid 100% CPU 
                    attempt++;
                }

                if (manager == nullptr) {
                    logger->error(fmt::format("Manager worker not initialized after maximum attempts, skipping index {}", indexmanager));
                    continue;  // Skip sending results if manager is still null 
                }

                try {
                    send_result(manager, indexmanager);
                }
                catch (const std::exception& e) {
                    logger->error(fmt::format("Exception while sending results for manager at index {}: {}", indexmanager, e.what()));
                }
                catch (...) {
                    logger->error(fmt::format("Unknown exception while sending results for manager at index {}", indexmanager));
                }

                indexmanager++;
            }
        }
    }
    catch (const std::exception& e) {
        logger->critical("Exception in listen_for_result: {}", e.what());
        continueall = false;  
    }
    catch (...) {
        logger->critical("Unknown exception in listen_for_result, terminating thread");
        continueall = false;
    }

    std::cout << "[Supervisor] End listen_for_result" << std::endl;
    logger->info("[Supervisor] End listen_for_result", globalname);
}

// Send result data
void Supervisor::send_result(WorkerManager* manager, int indexmanager) {
    if (manager->getResultLpQueue()->empty() && manager->getResultHpQueue()->empty()) {
        return;
    }

    json data;
    int channel = -1;

    if (!manager->getResultHpQueue()->empty()) {
        channel = 1;
        data = manager->getResultHpQueue()->get();
    }
    else if (!manager->getResultLpQueue()->empty()) {
        channel = 0;
        data = manager->getResultLpQueue()->get();
    }
    else {
        std::cerr << "Both queues are empty, can't send results." << std::endl;
        logger->warning("Both queues are empty, can't send results.");
        return;
    }

    if (channel == 0) {
        logger->info("Sending lp results.");

        if (manager->get_result_lp_socket() == "none") {
            std::cerr << "Lp socket is empty, can't send results." << std::endl;
            logger->warning("Lp socket is empty, can't send results.");
            return;
        }
        if (manager->get_result_dataflow_type() == "string" || manager->get_result_dataflow_type() == "filename") {
            try {
                std::string data_str = data.get<std::string>();
                socket_lp_result[indexmanager]->send(zmq::buffer(data_str));
            }
            catch (const std::exception& e) {
                std::cerr << "ERROR: data not in string format to be sent to: " << e.what() << std::endl;
                logger->error("ERROR: data not in string format to be sent to: " + std::string(e.what()), globalname);
            }
        }
        else if (manager->get_result_dataflow_type() == "binary") {
            try {
                logger->info("Supervisor::send_result: sending binary lp results.");
                socket_lp_result[indexmanager]->send(zmq::buffer(data.dump()));
                logger->info("Supervisor::send_result: finished sending binary lp results.");
            }
            catch (const std::exception& e) {
                std::cerr << "ERROR: data not in binary format to be sent to socket_result: " << e.what() << std::endl;
                logger->error("ERROR: data not in binary format to be sent to socket_result: " + std::string(e.what()), globalname);
            }
        }
    }

    if (channel == 1) {
        logger->info("Sending hp results.");

        if (manager->get_result_hp_socket() == "none") {
            std::cerr << "Hp socket is empty, can't send results." << std::endl;
            logger->warning("Hp socket is empty, can't send results.");
            return;
        }
        if (manager->get_result_dataflow_type() == "string" || manager->get_result_dataflow_type() == "filename") {
            try {
                std::string data_str = data.get<std::string>();
                socket_hp_result[indexmanager]->send(zmq::buffer(data_str));
            }
            catch (const std::exception& e) {
                std::cerr << "ERROR: data not in string format to be sent to: " << e.what() << std::endl;
                logger->error("ERROR: data not in string format to be sent to: " + std::string(e.what()), globalname);
            }
        }
        else if (manager->get_result_dataflow_type() == "binary") {
            try {
                socket_hp_result[indexmanager]->send(zmq::buffer(data.dump()));
            }
            catch (const std::exception& e) {
                std::cerr << "ERROR: data not in binary format to be sent to socket_result: " << e.what() << std::endl;
                logger->error("ERROR: data not in binary format to be sent to socket_result: " + std::string(e.what()), globalname);
            }
        }
    }
}

// Listen for low priority data (method is overridden in Supervisor1 and Supervisor2)
void Supervisor::listen_for_lp_data() {
    while (continueall) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 

        if (!stopdata) {
            zmq::message_t data;
            socket_lp_data->recv(data);
        }
    }

    std::cout << "[Supervisor] End listen_for_lp_data" << std::endl;
    logger->info("[Supervisor] End listen_for_lp_data", globalname);
}

// Listen for high priority binary data
void Supervisor::listen_for_hp_data() {
    while (continueall) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 

        if (!stopdata) {
            zmq::message_t data;
            socket_hp_data->recv(data);
        }
    }

    std::cout << "[Supervisor] End listen_for_hp_data" << std::endl;
    logger->info("[Supervisor] End listen_for_hp_data", globalname);
}

// Listen for low priority strings
void Supervisor::listen_for_lp_string() {
    while (continueall) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 

        if (!stopdata) {
            zmq::message_t data;
            socket_lp_data->recv(data);
            std::string data_str(static_cast<char*>(data.data()), data.size());
            std::vector<unsigned char> data_vec(data_str.begin(), data_str.end());

            for (auto& manager : manager_workers) {
                manager->getLowPriorityQueue()->push(data_vec);
            }
        }
    }

    std::cout << "End listen_for_lp_string" << std::endl;
    logger->info("End listen_for_lp_string", globalname);
}

// Listen for high priority strings
void Supervisor::listen_for_hp_string() {
    while (continueall) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 

        if (!stopdata) {
            zmq::message_t data;
            socket_hp_data->recv(data);
            std::string data_str(static_cast<char*>(data.data()), data.size());
            std::vector<unsigned char> data_vec(data_str.begin(), data_str.end());

            for (auto& manager : manager_workers) {
                manager->getHighPriorityQueue()->push(data_vec);
            }
        }
    }

    std::cout << "End listen_for_hp_string" << std::endl;
    logger->info("End listen_for_hp_string", globalname);
}

// Listen for low priority files
void Supervisor::listen_for_lp_file() {
    while (continueall) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 

        if (!stopdata) {
            zmq::message_t filename_msg;
            socket_lp_data->recv(filename_msg);
            std::string filename(static_cast<char*>(filename_msg.data()), filename_msg.size());

            for (auto& manager : manager_workers) {
                auto [data, size] = open_file(filename);
                for (int i = 0; i < size; i++) {
                    manager->getLowPriorityQueue()->push(data[i]);
                }
            }
        }
    }

    std::cout << "End listen_for_lp_file" << std::endl;
    logger->info("End listen_for_lp_file", globalname);
}

std::pair<std::vector<json>, int> Supervisor::open_file(const std::string& filename) {
    std::vector<json> data;  // Vector to store parsed JSON objects
    int size = 0;

    std::ifstream file(filename); // Open the file for reading
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << filename << std::endl;
        logger->error("Unable to open file: " + filename, globalname);
        return { data, size };  // Return empty vector and size 0 if the file cannot be opened
    }

    try {
        std::string line;
        while (std::getline(file, line)) { // Read the file line-by-line
            if (!line.empty()) { // Only attempt to parse non-empty lines
                json jsonData = json::parse(line); // Parse the line as JSON
                data.push_back(jsonData); // Add the parsed JSON object to the vector
                size++;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error while reading file: " << e.what() << std::endl;
        logger->error("Error while reading file: " + std::string(e.what()), globalname);
    }

    file.close(); // Close the file after reading
    return { data, size }; // Return the vector and the size of the data read
}

// Listen for high priority files
void Supervisor::listen_for_hp_file() {
    while (continueall) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 

        if (!stopdata) {
            zmq::message_t filename_msg;
            socket_hp_data->recv(filename_msg);
            std::string filename(static_cast<char*>(filename_msg.data()), filename_msg.size());

            for (auto& manager : manager_workers) {
                auto [data, size] = open_file(filename);
                for (int i = 0; i < size; i++) {
                    manager->getHighPriorityQueue()->push(data[i]);
                }
            }
        }
    }

    std::cout << "End listen_for_hp_file" << std::endl;
    logger->info("End listen_for_hp_file", globalname);
}

void Supervisor::listen_for_commands() {
    std::cout << "[Supervisor] Waiting for commands..." << std::endl;

    if (!socket_command) {
        logger->error("Socket is null or invalid in listen_for_commands");
        continueall = false;
    }

    while (continueall) {
        zmq::recv_flags flags = zmq::recv_flags::none;
        zmq::message_t command_msg;
        int err_code = zmq_errno();

        try {
            auto result = socket_command->recv(command_msg, flags);

            if (!result) {
                if (err_code == EAGAIN) {   // Continue if no commands were received
                    continue;
                }
                else if (err_code == EINTR) {   
                    break;
                }
                else {
                    logger->error("ZMQ recv error: {}", zmq_strerror(err_code));
                    break;
                }

                continue; // Keep looking for commands
            }
            else {
                if (err_code == EINTR) {    
                    break;
                }

                std::string command_str(static_cast<char*>(command_msg.data()), command_msg.size());

                try {
                    json command = json::parse(command_str);
                    process_command(command);
                }
                catch (const json::parse_error& e) {
                    logger->error("JSON parse error: {}", e.what());
                }
            }
        }
        catch (const zmq::error_t& e) {
            if (err_code == EINTR || e.num() == ETERM || e.num() == EINTR) {        // SIGINT
                continueall = false;
                break;
            }
            else {
                logger->error("ZMQ exception in listen_for_commands: {}", e.what());
                throw;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // To avoid 100% CPU 
    }

    std::cout << "[Supervisor] End listen_for_commands" << std::endl;
    logger->info("[Supervisor] End listen_for_commands", globalname);
}

// Shutdown command
void Supervisor::command_shutdown() {
    status = "Shutdown";
    stop_all(false);
}

// Cleaned shutdown command
void Supervisor::command_cleanedshutdown() {
    if (status == "Processing") {
        status = "EndingProcessing";
        command_stopdata();

        for (auto& manager : manager_workers) {
            std::cout << "[Supervisor] Trying to stop " << manager->get_globalname() << "..." << std::endl;
            logger->info("[Supervisor] Trying to stop " + manager->get_globalname() + "...", globalname);

            while (manager->getLowPriorityQueue()->size() != 0 || manager->getHighPriorityQueue()->size() != 0) {
                std::cout << "[Supervisor] Queues data of manager " << manager->get_globalname() << " have size "
                    << manager->getLowPriorityQueue()->size() << " " << manager->getHighPriorityQueue()->size() << std::endl;
                logger->info("[Supervisor] Queues data of manager " + manager->get_globalname() + " have size "
                    + std::to_string(manager->getLowPriorityQueue()->size()) + " "
                    + std::to_string(manager->getHighPriorityQueue()->size()), globalname);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            while (manager->getResultLpQueue()->size() != 0 || manager->getResultHpQueue()->size() != 0) {
                std::cout << "[Supervisor] Queues result of manager " << manager->get_globalname() << " have size "
                    << manager->getResultLpQueue()->size() << " " << manager->getResultHpQueue()->size() << std::endl;
                logger->info("[Supervisor] Queues result of manager " + manager->get_globalname() + " have size "
                    + std::to_string(manager->getResultLpQueue()->size()) + " "
                    + std::to_string(manager->getResultHpQueue()->size()), globalname);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
    }
    else {
        std::cerr << "[Supervisor] WARNING! Not in Processing state for a clean shutdown. Force the shutdown." << std::endl;
        logger->warning("[Supervisor] WARNING! Not in Processing state for a clean shutdown. Force the shutdown.", globalname);
    }

    status = "Shutdown";
    stop_all(false);
}

// Reset command
void Supervisor::command_reset() {
    if (status == "Processing" || status == "Waiting") {
        command_stop();

        for (auto& manager : manager_workers) {
            std::cout << "Trying to reset " << manager->get_globalname() << "..." << std::endl;
            logger->info("Trying to reset " + manager->get_globalname() + "...", globalname);
            manager->clean_queue();
            std::cout << "Queues of manager " << manager->get_globalname() << " have size "
                << manager->getLowPriorityQueue()->size() << " " << manager->getHighPriorityQueue()->size() << " "
                << manager->getResultLpQueue()->size() << " " << manager->getResultHpQueue()->size() << std::endl;
            logger->info("Queues of manager " + manager->get_globalname() + " have size "
                + std::to_string(manager->getLowPriorityQueue()->size()) + " "
                + std::to_string(manager->getHighPriorityQueue()->size()) + " "
                + std::to_string(manager->getResultLpQueue()->size()) + " "
                + std::to_string(manager->getResultHpQueue()->size()), globalname);
        }

        status = "Waiting";
        send_info(1, status, fullname, 1, "Low");
    }
}

// Start command
void Supervisor::command_start() {

    start_custom();
    command_startprocessing();
    command_startdata();
}

// Stop command
void Supervisor::command_stop() {
    command_stopdata();
    command_stopprocessing();
}

// Start processing command
void Supervisor::command_startprocessing() {
    status = "Processing";
    send_info(1, status, fullname, 1, "Low");

    for (auto& manager : manager_workers) {
        manager->set_processdata(1);
    }
}

// Stop processing command
void Supervisor::command_stopprocessing() {
    status = "Waiting";
    send_info(1, status, fullname, 1, "Low");

    for (auto& manager : manager_workers) {
        manager->set_processdata(0);
    }
}

// Start data command
void Supervisor::command_startdata() {
    stopdata = false;

    for (auto& manager : manager_workers) {
        manager->set_stopdata(false);
    }
}

// Stop data command
void Supervisor::command_stopdata() {
    stopdata = true;

    for (auto& manager : manager_workers) {
        manager->set_stopdata(true);
    }
}

// Process received commands
void Supervisor::process_command(const json& command) {
    int type_value = command["header"]["type"].get<int>();
    std::string subtype_value = command["header"]["subtype"].get<std::string>();
    std::string pidtarget = command["header"]["pidtarget"].get<std::string>();
    std::string pidsource = command["header"]["pidsource"].get<std::string>();

    if (type_value == 0) { // command
        if (pidtarget == name || pidtarget == "all" || pidtarget == "*") {
            std::cout << "\n[Supervisor] Received command: " << command << std::endl;
            if (subtype_value == "shutdown") {
                command_shutdown();
            }
            else if (subtype_value == "cleanedshutdown") {
                command_cleanedshutdown();
            }
            else if (subtype_value == "getstatus") {
                for (auto& manager : manager_workers) {
                    manager->getMonitoringThread()->sendto(pidsource);
                }
            }
            else if (subtype_value == "start") {
                command_start();
            }
            else if (subtype_value == "stop") {
                command_stop();
            }
            else if (subtype_value == "startprocessing") {
                command_startprocessing();
            }
            else if (subtype_value == "stopprocessing") {
                command_stopprocessing();
            }
            else if (subtype_value == "reset") {
                command_reset();
            }
            else if (subtype_value == "stopdata") {
                command_stopdata();
            }
            else if (subtype_value == "startdata") {
                command_startdata();
            }
        }
    }
    else if (type_value == 3) { // config
        for (auto& manager : manager_workers) {
            manager->configworkers(command);
        }
    }
}

// Send alarm message
void Supervisor::send_alarm(int level, const std::string& message, const std::string& pidsource, int code, const std::string& priority) {
    json msg;
    msg["header"]["type"] = 2;
    msg["header"]["subtype"] = "alarm";
    msg["header"]["time"] = static_cast<double>(time(nullptr));
    msg["header"]["pidsource"] = pidsource;
    msg["header"]["pidtarget"] = "*";
    msg["header"]["priority"] = priority;
    msg["body"]["level"] = level;
    msg["body"]["code"] = code;
    msg["body"]["message"] = message;
    socket_monitoring->send(zmq::buffer(msg.dump()));
}

// Send log message
void Supervisor::send_log(int level, const std::string& message, const std::string& pidsource, int code, const std::string& priority) {
    json msg;
    msg["header"]["type"] = 4;
    msg["header"]["subtype"] = "log";
    msg["header"]["time"] = static_cast<double>(time(nullptr));
    msg["header"]["pidsource"] = pidsource;
    msg["header"]["pidtarget"] = "*";
    msg["header"]["priority"] = priority;
    msg["body"]["level"] = level;
    msg["body"]["code"] = code;
    msg["body"]["message"] = message;
    socket_monitoring->send(zmq::buffer(msg.dump()));
}

// Send info message
void Supervisor::send_info(int level, const std::string& message, const std::string& pidsource, int code, const std::string& priority) {
    json msg;
    msg["header"]["type"] = 5;
    msg["header"]["subtype"] = "info";
    msg["header"]["time"] = static_cast<double>(time(nullptr));
    msg["header"]["pidsource"] = pidsource;
    msg["header"]["pidtarget"] = "*";
    msg["header"]["priority"] = priority;
    msg["body"]["level"] = level;
    msg["body"]["code"] = code;
    msg["body"]["message"] = message;
    socket_monitoring->send(zmq::buffer(msg.dump()));
}

// Stop all threads and processes
void Supervisor::stop_all(bool fast) {
    continueall = false;

    std::cout << "[Supervisor] Stopping all workers and managers..." << std::endl;
    logger->info("[Supervisor] Stopping all workers and managers...", globalname);

    // Stop data processing custom logic if implemented
    stop_custom();

    command_stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto& manager : manager_workers) {
        manager->stop(fast);
    }

    // continueall = false;

    std::cout << "[Supervisor] All Supervisor workers and managers and internal threads terminated." << std::endl;
    logger->info("[Supervisor] All Supervisor workers and managers and internal threads terminated.", globalname);
}

void buildDefaultA0Packet(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID = 0) {
    if (maxBufferSize < sizeof(Header) + sizeof(Data_Header)) {
        printf("Error: buffer too small in buildDefaultA0Packet\n");
        return;
    }

    HeaderDams* header = (HeaderDams*)buffer;
    header->start = HeaderDams::START;
    header->apid = HeaderDams::CLASS_TC | 0x01;  // SOURCE = 1 esempio
    header->sequence = HeaderDams::GROUP_STAND_ALONE | 0x0001; // esempio count 1
    header->runID = runID;
    header->size = sizeof(Data_Header); // dati utili dopo header
    header->crc = 0; // se hai CRC calcolalo poi!
    header->encode();

    Data_Header* data = (Data_Header*)(buffer + sizeof(HeaderDams));
    data->type = 0xA0;
    data->subType = 0x99; // caso "non definito" → farà trigger FE
}

void buildStartAcqPacket(uint8_t* buffer, const size_t maxBufferSize, uint16_t runID = 0) {
    if (maxBufferSize < sizeof(HeaderDams) + sizeof(Data_Header)) {
        printf("Error: buffer too small in buildStartAcqPacket\n");
        return;
    }

    HeaderDams* header = (HeaderDams*)buffer;
    header->start = HeaderDams::START;
    header->apid = HeaderDams::CLASS_TC | 0x01;  // SOURCE = 1 esempio
    header->sequence = HeaderDams::GROUP_STAND_ALONE | 0x0002; // esempio count 2
    header->runID = runID;
    header->size = sizeof(Data_Header);
    header->crc = 0; // se hai CRC calcolalo poi!
    header->encode();

    Data_Header* data = (Data_Header*)(buffer + sizeof(HeaderDams));
    data->type = 0xA0;
    data->subType = 0x04; // StartAcq
}