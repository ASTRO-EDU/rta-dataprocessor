#include <iostream>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <json/json.h>
#include <thread>
#include <queue>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <psutil.h> // Assume a suitable library for psutil functions
#include "WorkerLogger.h"
#include "ConfigurationManager.h"
#include "WorkerManager.h"

// Supervisor class definition
class Supervisor {
public:
    Supervisor(std::string config_file = "config.json", std::string name = "None") 
        : name(name), continueall(true) {
        // Initialize member variables
        config_manager = nullptr;
        manager_num_workers = 0;

        load_configuration(config_file, name);
        fullname = name;
        globalname = "Supervisor-" + name;

        // Set up logging
        std::string log_file = config.get("logs_path").asString() + "/" + globalname + ".log";
        logger = new WorkerLogger("worker_logger", log_file, 10);

        pid = getpid();

        context = zmq::context_t(1);

        try {
            // Retrieve and log configuration
            processingtype = config.get("processing_type").asString();
            dataflowtype = config.get("dataflow_type").asString();
            datasockettype = config.get("datasocket_type").asString();

            std::cout << "Supervisor: " << globalname << " / " << dataflowtype << " / " 
                      << processingtype << " / " << datasockettype << std::endl;
            logger->system("Supervisor: " + globalname + " / " + dataflowtype + " / " 
                           + processingtype + " / " + datasockettype, globalname);

            // Set up data sockets based on configuration
            if (datasockettype == "pushpull") {
                socket_lp_data = new zmq::socket_t(context, ZMQ_PULL);
                socket_lp_data->bind(get_pull_config(config.get("data_lp_socket")));

                socket_hp_data = new zmq::socket_t(context, ZMQ_PULL);
                socket_hp_data->bind(get_pull_config(config.get("data_hp_socket")));
            } else if (datasockettype == "pubsub") {
                socket_lp_data = new zmq::socket_t(context, ZMQ_SUB);
                socket_lp_data->connect(config.get("data_lp_socket").asString());
                socket_lp_data->setsockopt(ZMQ_SUBSCRIBE, "", 0);

                socket_hp_data = new zmq::socket_t(context, ZMQ_SUB);
                socket_hp_data->connect(config.get("data_hp_socket").asString());
                socket_hp_data->setsockopt(ZMQ_SUBSCRIBE, "", 0);
            } else if (datasockettype == "custom") {
                logger->system("Supervisor started with custom data receiver", globalname);
            } else {
                throw std::invalid_argument("Config file: datasockettype must be pushpull or pubsub");
            }

            // Set up command and monitoring sockets
            socket_command = new zmq::socket_t(context, ZMQ_SUB);
            socket_command->connect(config.get("command_socket").asString());
            socket_command->setsockopt(ZMQ_SUBSCRIBE, "", 0);

            socket_monitoring = new zmq::socket_t(context, ZMQ_PUSH);
            socket_monitoring->connect(config.get("monitoring_socket").asString());

            socket_lp_result.resize(100, nullptr);
            socket_hp_result.resize(100, nullptr);
        } catch (const std::exception &e) {
            // Handle any other unexpected exceptions
            std::cerr << "ERROR: An unexpected error occurred: " << e.what() << std::endl;
            logger->warning("ERROR: An unexpected error occurred: " + std::string(e.what()), globalname);
            exit(1);
        }

        manager_workers = std::vector<WorkerManager*>();

        processdata = 0;
        stopdata = true;

        // Set up signal handlers
        try {
            signal(SIGTERM, handle_signals);
            signal(SIGINT, handle_signals);
        } catch (const std::exception &e) {
            std::cerr << "WARNING! Signal only works in main thread. It is not possible to set up signal handlers!" << std::endl;
            logger->warning("WARNING! Signal only works in main thread. It is not possible to set up signal handlers!", globalname);
        }

        status = "Initialised";
        send_info(1, status, fullname, 1, "Low");

        std::cout << globalname << " started" << std::endl;
        logger->system(globalname + " started", globalname);
    }

    // Destructor
    ~Supervisor() {
        delete socket_lp_data;
        delete socket_hp_data;
        delete socket_command;
        delete socket_monitoring;
        delete logger;
    }

    // Load configuration
    void load_configuration(const std::string &config_file, const std::string &name) {
        config_manager = new ConfigurationManager(config_file);
        config = config_manager->get_configuration(name);
        std::cout << config << std::endl;
        std::tie(manager_result_sockets_type, manager_result_dataflow_type, 
                 manager_result_lp_sockets, manager_result_hp_sockets, 
                 manager_num_workers, workername, name_workers) = config_manager->get_workers_config(name);
    }

    // Start service threads
    void start_service_threads() {
        if (dataflowtype == "binary") {
            lp_data_thread = std::thread(&Supervisor::listen_for_lp_data, this);
            hp_data_thread = std::thread(&Supervisor::listen_for_hp_data, this);
        } else if (dataflowtype == "filename") {
            lp_data_thread = std::thread(&Supervisor::listen_for_lp_file, this);
            hp_data_thread = std::thread(&Supervisor::listen_for_hp_file, this);
        } else if (dataflowtype == "string") {
            lp_data_thread = std::thread(&Supervisor::listen_for_lp_string, this);
            hp_data_thread = std::thread(&Supervisor::listen_for_hp_string, this);
        }

        result_thread = std::thread(&Supervisor::listen_for_result, this);
    }

    // Set up result channel
    void setup_result_channel(WorkerManager *manager, int indexmanager) {
        socket_lp_result[indexmanager] = nullptr;
        socket_hp_result[indexmanager] = nullptr;
        context = zmq::context_t(1);

        if (manager->result_lp_socket != "none") {
            if (manager->result_socket_type == "pushpull") {
                socket_lp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUSH);
                socket_lp_result[indexmanager]->connect(manager->result_lp_socket);
                std::cout << "---result lp socket pushpull " << manager->globalname << " " << manager->result_lp_socket << std::endl;
                logger->system("---result lp socket pushpull " + manager->globalname + " " + manager->result_lp_socket, globalname);
            } else if (manager->result_socket_type == "pubsub") {
                socket_lp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUB);
                socket_lp_result[indexmanager]->bind(manager->result_lp_socket);
                std::cout << "---result lp socket pushpull " << manager->globalname << " " << manager->result_lp_socket << std::endl;
                logger->system("---result lp socket pushpull " + manager->globalname + " " + manager->result_lp_socket, globalname);
            }
        }

        if (manager->result_hp_socket != "none") {
            if (manager->result_socket_type == "pushpull") {
                socket_hp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUSH);
                socket_hp_result[indexmanager]->connect(manager->result_hp_socket);
                std::cout << "---result hp socket pushpull " << manager->globalname << " " << manager->result_hp_socket << std::endl;
                logger->system("---result hp socket pushpull " + manager->globalname + " " + manager->result_hp_socket, globalname);
            } else if (manager->result_socket_type == "pubsub") {
                socket_hp_result[indexmanager] = new zmq::socket_t(context, ZMQ_PUB);
                socket_hp_result[indexmanager]->bind(manager->result_hp_socket);
                std::cout << "---result hp socket pushpull " << manager->globalname << " " << manager->result_hp_socket << std::endl;
                logger->system("---result hp socket pushpull " + manager->globalname + " " + manager->result_hp_socket, globalname);
            }
        }
    }

    // Start managers
    void start_managers() {
        int indexmanager = 0;
        WorkerManager *manager = new WorkerManager(indexmanager, this, "Generic");
        setup_result_channel(manager, indexmanager);
        manager->start();
        manager_workers.push_back(manager);
    }

    // Start workers
    void start_workers() {
        int indexmanager = 0;
        for (auto &manager : manager_workers) {
            if (processingtype == "thread" || processingtype == "process") {
                manager->start_worker_threads(manager_num_workers);
            }
            indexmanager++;
        }
    }

    // Start Supervisor
    void start() {
        start_service_threads();
        start_managers();
        start_workers();

        status = "Waiting";
        send_info(1, status, fullname, 1, "Low");

        try {
            while (continueall) {
                listen_for_commands();
                std::this_thread::sleep_for(std::chrono::seconds(1)); // To avoid 100% CPU
            }
        } catch (const std::exception &e) {
            std::cerr << "Exception caught: " << e.what() << std::endl;
            command_shutdown();
        }
    }

    // Signal handler
    static void handle_signals(int signum) {
        if (signum == SIGTERM) {
            std::cerr << "SIGTERM received. Terminating with cleanedshutdown." << std::endl;
            logger->system("SIGTERM received. Terminating with cleanedshutdown", globalname);
            command_cleanedshutdown();
        } else if (signum == SIGINT) {
            std::cerr << "SIGINT received. Terminating with shutdown." << std::endl;
            logger->system("SIGINT received. Terminating with shutdown", globalname);
            command_shutdown();
        } else {
            std::cerr << "Received signal " << signum << ". Terminating." << std::endl;
            logger->system("Received signal " + std::to_string(signum) + ". Terminating", globalname);
            command_shutdown();
        }
    }

    // Function to listen for results
    void listen_for_result() {
        while (continueall) {
            int indexmanager = 0;
            for (auto &manager : manager_workers) {
                send_result(manager, indexmanager);
                indexmanager++;
            }
        }
        std::cout << "End listen_for_result" << std::endl;
        logger->system("End listen_for_result", globalname);
    }

    // Function to send result
    void send_result(WorkerManager *manager, int indexmanager) {
        if (manager->result_lp_queue.size() == 0 && manager->result_hp_queue.size() == 0) {
            return;
        }

        Json::Value data;
        int channel = -1;
        try {
            channel = 1;
            data = manager->result_hp_queue.front();
            manager->result_hp_queue.pop();
        } catch (const std::exception &e) {
            try {
                channel = 0;
                data = manager->result_lp_queue.front();
                manager->result_lp_queue.pop();
            } catch (const std::exception &e) {
                return;
            }
        }

        if (channel == 0) {
            if (manager->result_lp_socket == "none") {
                return;
            }
            if (manager->result_dataflow_type == "string" || manager->result_dataflow_type == "filename") {
                try {
                    std::string data_str = data.asString();
                    socket_lp_result[indexmanager]->send(zmq::buffer(data_str));
                } catch (const std::exception &e) {
                    std::cerr << "ERROR: data not in string format to be send to: " << e.what() << std::endl;
                    logger->error("ERROR: data not in string format to be send to: " + std::string(e.what()), globalname);
                }
            } else if (manager->result_dataflow_type == "binary") {
                try {
                    socket_lp_result[indexmanager]->send(zmq::buffer(data.toStyledString()));
                } catch (const std::exception &e) {
                    std::cerr << "ERROR: data not in binary format to be send to socket_result: " << e.what() << std::endl;
                    logger->error("ERROR: data not in binary format to be send to socket_result: " + std::string(e.what()), globalname);
                }
            }
        }

        if (channel == 1) {
            if (manager->result_hp_socket == "none") {
                return;
            }
            if (manager->result_dataflow_type == "string" || manager->result_dataflow_type == "filename") {
                try {
                    std::string data_str = data.asString();
                    socket_hp_result[indexmanager]->send(zmq::buffer(data_str));
                } catch (const std::exception &e) {
                    std::cerr << "ERROR: data not in string format to be send to: " << e.what() << std::endl;
                    logger->error("ERROR: data not in string format to be send to: " + std::string(e.what()), globalname);
                }
            } else if (manager->result_dataflow_type == "binary") {
                try {
                    socket_hp_result[indexmanager]->send(zmq::buffer(data.toStyledString()));
                } catch (const std::exception &e) {
                    std::cerr << "ERROR: data not in binary format to be send to socket_result: " << e.what() << std::endl;
                    logger->error("ERROR: data not in binary format to be send to socket_result: " + std::string(e.what()), globalname);
                }
            }
        }
    }

    // Function to listen for low priority data
    void listen_for_lp_data() {
        while (continueall) {
            if (!stopdata) {
                zmq::message_t data;
                socket_lp_data->recv(data);
                for (auto &manager : manager_workers) {
                    Json::Value decodeddata = decode_data(data);
                    manager->low_priority_queue.push(decodeddata);
                }
            }
        }
        std::cout << "End listen_for_lp_data" << std::endl;
        logger->system("End listen_for_lp_data", globalname);
    }

    // Function to listen for high priority data
    void listen_for_hp_data() {
        while (continueall) {
            if (!stopdata) {
                zmq::message_t data;
                socket_hp_data->recv(data);
                for (auto &manager : manager_workers) {
                    Json::Value decodeddata = decode_data(data);
                    manager->high_priority_queue.push(decodeddata);
                }
            }
        }
        std::cout << "End listen_for_hp_data" << std::endl;
        logger->system("End listen_for_hp_data", globalname);
    }

    // Function to listen for low priority strings
    void listen_for_lp_string() {
        while (continueall) {
            if (!stopdata) {
                zmq::message_t data;
                socket_lp_data->recv(data);
                std::string data_str(static_cast<char*>(data.data()), data.size());
                for (auto &manager : manager_workers) {
                    manager->low_priority_queue.push(data_str);
                }
            }
        }
        std::cout << "End listen_for_lp_string" << std::endl;
        logger->system("End listen_for_lp_string", globalname);
    }

    // Function to listen for high priority strings
    void listen_for_hp_string() {
        while (continueall) {
            if (!stopdata) {
                zmq::message_t data;
                socket_hp_data->recv(data);
                std::string data_str(static_cast<char*>(data.data()), data.size());
                for (auto &manager : manager_workers) {
                    manager->high_priority_queue.push(data_str);
                }
            }
        }
        std::cout << "End listen_for_hp_string" << std::endl;
        logger->system("End listen_for_hp_string", globalname);
    }

    // Function to listen for low priority files
    void listen_for_lp_file() {
        while (continueall) {
            if (!stopdata) {
                zmq::message_t filename_msg;
                socket_lp_data->recv(filename_msg);
                std::string filename(static_cast<char*>(filename_msg.data()), filename_msg.size());
                for (auto &manager : manager_workers) {
                    auto [data, size] = open_file(filename);
                    for (int i = 0; i < size; i++) {
                        manager->low_priority_queue.push(data[i]);
                    }
                }
            }
        }
        std::cout << "End listen_for_lp_file" << std::endl;
        logger->system("End listen_for_lp_file", globalname);
    }

    // Function to listen for high priority files
    void listen_for_hp_file() {
        while (continueall) {
            if (!stopdata) {
                zmq::message_t filename_msg;
                socket_hp_data->recv(filename_msg);
                std::string filename(static_cast<char*>(filename_msg.data()), filename_msg.size());
                for (auto &manager : manager_workers) {
                    auto [data, size] = open_file(filename);
                    for (int i = 0; i < size; i++) {
                        manager->high_priority_queue.push(data[i]);
                    }
                }
            }
        }
        std::cout << "End listen_for_hp_file" << std::endl;
        logger->system("End listen_for_hp_file", globalname);
    }

    // Function to listen for commands
    void listen_for_commands() {
        while (continueall) {
            std::cout << "Waiting for commands..." << std::endl;
            logger->system("Waiting for commands...", globalname);

            zmq::message_t command_msg;
            socket_command->recv(command_msg);
            std::string command_str(static_cast<char*>(command_msg.data()), command_msg.size());
            Json::Value command;
            Json::CharReaderBuilder reader;
            std::string errs;
            std::istringstream s(command_str);
            if (Json::parseFromStream(reader, s, &command, &errs)) {
                process_command(command);
            }
        }
        std::cout << "End listen_for_commands" << std::endl;
        logger->system("End listen_for_commands", globalname);
    }

    // Shutdown command
    void command_shutdown() {
        status = "Shutdown";
        stop_all(false);
    }

    // Cleaned shutdown command
    void command_cleanedshutdown() {
        if (status == "Processing") {
            status = "EndingProcessing";
            command_stopdata();
            for (auto &manager : manager_workers) {
                std::cout << "Trying to stop " << manager->globalname << "..." << std::endl;
                logger->system("Trying to stop " + manager->globalname + "...", globalname);
                while (manager->low_priority_queue.size() != 0 || manager->high_priority_queue.size() != 0) {
                    std::cout << "Queues data of manager " << manager->globalname << " have size " 
                              << manager->low_priority_queue.size() << " " << manager->high_priority_queue.size() << std::endl;
                    logger->system("Queues data of manager " + manager->globalname + " have size " 
                                   + std::to_string(manager->low_priority_queue.size()) + " " 
                                   + std::to_string(manager->high_priority_queue.size()), globalname);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
                while (manager->result_lp_queue.size() != 0 || manager->result_hp_queue.size() != 0) {
                    std::cout << "Queues result of manager " << manager->globalname << " have size " 
                              << manager->result_lp_queue.size() << " " << manager->result_hp_queue.size() << std::endl;
                    logger->system("Queues result of manager " + manager->globalname + " have size " 
                                   + std::to_string(manager->result_lp_queue.size()) + " " 
                                   + std::to_string(manager->result_hp_queue.size()), globalname);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
        } else {
            std::cerr << "WARNING! Not in Processing state for a cleaned shutdown. Force the shutdown." << std::endl;
            logger->warning("WARNING! Not in Processing state for a cleaned shutdown. Force the shutdown.", globalname);
        }

        status = "Shutdown";
        stop_all(false);
    }

    // Reset command
    void command_reset() {
        if (status == "Processing" || status == "Waiting") {
            command_stop();
            for (auto &manager : manager_workers) {
                std::cout << "Trying to reset " << manager->globalname << "..." << std::endl;
                logger->system("Trying to reset " + manager->globalname + "...", globalname);
                manager->clean_queue();
                std::cout << "Queues of manager " << manager->globalname << " have size " 
                          << manager->low_priority_queue.size() << " " << manager->high_priority_queue.size() << " " 
                          << manager->result_lp_queue.size() << " " << manager->result_hp_queue.size() << std::endl;
                logger->system("Queues of manager " + manager->globalname + " have size " 
                               + std::to_string(manager->low_priority_queue.size()) + " " 
                               + std::to_string(manager->high_priority_queue.size()) + " " 
                               + std::to_string(manager->result_lp_queue.size()) + " " 
                               + std::to_string(manager->result_hp_queue.size()), globalname);
            }
            status = "Waiting";
            send_info(1, status, fullname, 1, "Low");
        }
    }

    // Start command
    void command_start() {
        command_startprocessing();
        command_startdata();
    }

    // Stop command
    void command_stop() {
        command_stopdata();
        command_stopprocessing();
    }

    // Start processing command
    void command_startprocessing() {
        status = "Processing";
        send_info(1, status, fullname, 1, "Low");
        for (auto &manager : manager_workers) {
            manager->set_processdata(1);
        }
    }

    // Stop processing command
    void command_stopprocessing() {
        status = "Waiting";
        send_info(1, status, fullname, 1, "Low");
        for (auto &manager : manager_workers) {
            manager->set_processdata(0);
        }
    }

    // Start data command
    void command_startdata() {
        stopdata = false;
        for (auto &manager : manager_workers) {
            manager->set_stopdata(false);
        }
    }

    // Stop data command
    void command_stopdata() {
        stopdata = true;
        for (auto &manager : manager_workers) {
            manager->set_stopdata(true);
        }
    }

    // Process command
    void process_command(const Json::Value &command) {
        int type_value = command["header"]["type"].asInt();
        std::string subtype_value = command["header"]["subtype"].asString();
        std::string pidtarget = command["header"]["pidtarget"].asString();
        std::string pidsource = command["header"]["pidsource"].asString();

        if (type_value == 0) { // command
            if (pidtarget == name || pidtarget == "all" || pidtarget == "*") {
                std::cout << "Received command: " << command << std::endl;
                if (subtype_value == "shutdown") {
                    command_shutdown();
                } else if (subtype_value == "cleanedshutdown") {
                    command_cleanedshutdown();
                } else if (subtype_value == "getstatus") {
                    for (auto &manager : manager_workers) {
                        manager->monitoring_thread.sendto(pidsource);
                    }
                } else if (subtype_value == "start") {
                    command_start();
                } else if (subtype_value == "stop") {
                    command_stop();
                } else if (subtype_value == "startprocessing") {
                    command_startprocessing();
                } else if (subtype_value == "stopprocessing") {
                    command_stopprocessing();
                } else if (subtype_value == "reset") {
                    command_reset();
                } else if (subtype_value == "stopdata") {
                    command_stopdata();
                } else if (subtype_value == "startdata") {
                    command_startdata();
                }
            }
        } else if (type_value == 3) { // config
            for (auto &manager : manager_workers) {
                manager->configworkers(command);
            }
        }
    }

    // Send alarm
    void send_alarm(int level, const std::string &message, const std::string &pidsource, int code = 0, const std::string &priority = "Low") {
        Json::Value msg;
        msg["header"]["type"] = 2;
        msg["header"]["subtype"] = "alarm";
        msg["header"]["time"] = static_cast<double>(time(nullptr));
        msg["header"]["pidsource"] = pidsource;
        msg["header"]["pidtarget"] = "*";
        msg["header"]["priority"] = priority;
        msg["body"]["level"] = level;
        msg["body"]["code"] = code;
        msg["body"]["message"] = message;
        socket_monitoring->send(zmq::buffer(msg.toStyledString()));
    }

    // Send log
    void send_log(int level, const std::string &message, const std::string &pidsource, int code = 0, const std::string &priority = "Low") {
        Json::Value msg;
        msg["header"]["type"] = 4;
        msg["header"]["subtype"] = "log";
        msg["header"]["time"] = static_cast<double>(time(nullptr));
        msg["header"]["pidsource"] = pidsource;
        msg["header"]["pidtarget"] = "*";
        msg["header"]["priority"] = priority;
        msg["body"]["level"] = level;
        msg["body"]["code"] = code;
        msg["body"]["message"] = message;
        socket_monitoring->send(zmq::buffer(msg.toStyledString()));
    }

    // Send info
    void send_info(int level, const std::string &message, const std::string &pidsource, int code = 0, const std::string &priority = "Low") {
        Json::Value msg;
        msg["header"]["type"] = 5;
        msg["header"]["subtype"] = "info";
        msg["header"]["time"] = static_cast<double>(time(nullptr));
        msg["header"]["pidsource"] = pidsource;
        msg["header"]["pidtarget"] = "*";
        msg["header"]["priority"] = priority;
        msg["body"]["level"] = level;
        msg["body"]["code"] = code;
        msg["body"]["message"] = message;
        socket_monitoring->send(zmq::buffer(msg.toStyledString()));
    }

    // Stop all threads and processes
    void stop_all(bool fast = false) {
        std::cout << "Stopping all workers and managers..." << std::endl;
        logger->system("Stopping all workers and managers...", globalname);

        command_stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        for (auto &manager : manager_workers) {
            manager->stop(fast);
            manager->join();
        }

        continueall = false;

        std::cout << "All Supervisor workers and managers and internal threads terminated." << std::endl;
        logger->system("All Supervisor workers and managers and internal threads terminated.", globalname);
    }

private:
    std::string name;
    std::string fullname;
    std::string globalname;
    bool continueall;
    int pid;
    zmq::context_t context;
    zmq::socket_t *socket_lp_data;
    zmq::socket_t *socket_hp_data;
    zmq::socket_t *socket_command;
    zmq::socket_t *socket_monitoring;
    std::vector<zmq::socket_t*> socket_lp_result;
    std::vector<zmq::socket_t*> socket_hp_result;
    WorkerLogger *logger;
    ConfigurationManager *config_manager;
    Json::Value config;
    int manager_num_workers;
    std::string manager_result_sockets_type;
    std::string manager_result_dataflow_type;
    std::vector<std::string> manager_result_lp_sockets;
    std::vector<std::string> manager_result_hp_sockets;
    std::string workername;
    std::vector<std::string> name_workers;
    std::string processingtype;
    std::string dataflowtype;
    std::string datasockettype;
    std::vector<WorkerManager*> manager_workers;
    int processdata;
    bool stopdata;
    std::string status;
    std::thread lp_data_thread;
    std::thread hp_data_thread;
    std::thread result_thread;
};

