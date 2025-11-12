#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include "json.hpp"

using json = nlohmann::json;


class ConfigurationManager {

    // Reads configurations from the specified file
    std::vector<json> read_configurations_from_file(const std::string& file_path);

    // Validates the configurations to ensure all required fields are present
    void validate_configurations(const std::vector<json>& configurations) const;

    // Creates an in-memory structure from the configurations
    std::map<std::string, json> create_memory_structure();

    // Holds the list of configurations read from the file
    std::vector<json> configurations;

    // Maps processor names to their corresponding configurations
    std::map<std::string, json> config;

    // List of required fields for each configuration
    const std::vector<std::string> REQUIRED_FIELDS = {
        "processname",
        "dataflow_type",
        "processing_type",
        "datasocket_type",
        "data_lp_socket",
        "data_hp_socket",
        "command_socket",
        "monitoring_socket",
        "logs_path",
        "logs_level",
        "comment"
    };

    // List of required fields for manager configurations
    const std::vector<std::string> MANAGER_FIELDS = {
        "result_socket_type",
        "result_dataflow_type",
        "result_lp_socket",
        "result_hp_socket",
        "num_workers",
        "name",
        "name_workers"
    };

public:
    // Constructor that initializes the ConfigurationManager with a file path
    ConfigurationManager(const std::string& file_path);

    // Returns the configuration for a specific processor name
    json get_configuration(const std::string& processorname) const;

    // Returns the worker configurations for a specific processor name as a tuple
    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<int>, std::vector<std::string>, std::vector<std::string>> get_workers_config(const std::string& processorname) const;


};

#endif // CONFIGURATIONMANAGER_H