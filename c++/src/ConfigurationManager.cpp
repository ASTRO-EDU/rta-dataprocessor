// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <rtadp/ConfigurationManager.h>

using json = nlohmann::json;

// Constructor that initializes the ConfigurationManager with a file path
ConfigurationManager::ConfigurationManager(const std::string& file_path) {
    configurations = read_configurations_from_file(file_path);
    if (!configurations.empty()) {
        // validate_configurations(configurations);
        config = create_memory_structure();
    }
}

// Reads configurations from the specified file
std::vector<json> ConfigurationManager::read_configurations_from_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: File '" << file_path << "' not found." << std::endl;
        return {};
    }

    json configurations;
    file >> configurations;
    if (configurations.is_null()) {
        std::cerr << "Error: Invalid JSON format in file '" << file_path << "'." << std::endl;
        return {};
    }

    std::vector<json> configs;
    for (const auto& config : configurations) {
        configs.push_back(config);
    }
    return configs;
}

// Validates the configurations to ensure all required fields are present
void ConfigurationManager::validate_configurations(const std::vector<json>& configurations) const {
    for (const auto& config : configurations) {
        std::cout << "Configuration: " << config.dump(10) << std::endl;
        for (const auto& field : REQUIRED_FIELDS) {
            if (!config.contains(field) || config[field].is_null()) {
                throw std::runtime_error("Field '" + field + "' is missing or not well-formed in one or more configurations."); //here
            }
        }
        for (const auto& manager : config["manager"]) {
            for (const auto& field : MANAGER_FIELDS) {
                if (!manager.contains(field) || manager[field].is_null()) {
                    throw std::runtime_error("Field '" + field + "' is missing or not well-formed in one or more manager configurations.");
                }
            }
        }
    }
}

// Creates an in-memory structure from the configurations
std::map<std::string, json> ConfigurationManager::create_memory_structure() {
    std::map<std::string, json> structure;
    for (const auto& config : configurations) {
        std::string processorname = config["processname"];
        structure[processorname] = config;
    }
    return structure;
}

// Returns the configuration for a specific processor name
json ConfigurationManager::get_configuration(const std::string& processorname) const {
    auto it = config.find(processorname);
    if (it != config.end()) {
        return it->second;
    }
    return {};
}

// Returns the worker configurations for a specific processor name as a tuple
std::tuple<
    std::vector<std::string>, 
    std::vector<std::string>, 
    std::vector<std::string>, 
    std::vector<std::string>, 
    std::vector<int>, 
    std::vector<std::string>, 
    std::vector<std::string>
> ConfigurationManager::get_workers_config(const std::string& processorname) const {
    json config = get_configuration(processorname);
    if (!config.is_null()) {
        std::vector<std::string> result_socket_type, result_dataflow_type, result_lp_sockets, result_hp_sockets, workername, name_workers;
        std::vector<int> num_workers;

        for (const auto& manager : config["manager"]) {
            result_socket_type.push_back(manager["result_socket_type"]);
            result_dataflow_type.push_back(manager["result_dataflow_type"]);
            result_lp_sockets.push_back(manager["result_lp_socket"]);
            result_hp_sockets.push_back(manager["result_hp_socket"]);
            num_workers.push_back(manager["num_workers"]);
            workername.push_back(manager["name"]);
            name_workers.push_back(manager["name_workers"]);
        }

        return { 
            result_socket_type, 
            result_dataflow_type, 
            result_lp_sockets, 
            result_hp_sockets, 
            num_workers, 
            workername, 
            name_workers 
        };
    }
    return { {}, {}, {}, {}, {}, {}, {} };
}