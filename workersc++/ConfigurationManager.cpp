// Copyright (C) 2024 INAF
// This software is distributed under the terms of the BSD-3-Clause license
//
// Authors:
//
//    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <json/json.h> // Assume a suitable JSON library

// Function to get custom config from an address
std::vector<std::string> get_custom_config(const std::string& address) {
    std::vector<std::string> parts;
    std::istringstream iss(address);
    std::string part;
    while (std::getline(iss, part, ':')) {
        parts.push_back(part);
    }

    if (parts.size() == 3) {
        return { parts[0] + ":" + parts[1], parts[2] };
    }
    return {};
}

// Function to get pull config from an address
std::string get_pull_config(const std::string& address) {
    std::vector<std::string> parts;
    std::istringstream iss(address);
    std::string part;
    while (std::getline(iss, part, ':')) {
        parts.push_back(part);
    }

    if (parts.size() == 3 && parts[0] == "tcp") {
        return parts[0] + "://*:" + parts[2];
    }
    return "";
}

class ConfigurationManager {
public:
    // Constructor to initialize ConfigurationManager with a file path
    ConfigurationManager(const std::string& file_path) {
        configurations = read_configurations_from_file(file_path);
        if (!configurations.empty()) {
            config = create_memory_structure();
        }
    }

    // Function to read configurations from a file
    std::vector<Json::Value> read_configurations_from_file(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "Error: File '" << file_path << "' not found." << std::endl;
            return {};
        }

        Json::Value configurations;
        file >> configurations;
        if (configurations.isNull()) {
            std::cerr << "Error: Invalid JSON format in file '" << file_path << "'." << std::endl;
            return {};
        }

        std::vector<Json::Value> configs;
        for (const auto& config : configurations) {
            configs.push_back(config);
        }
        return configs;
    }

    // Function to create memory structure from configurations
    std::map<std::string, Json::Value> create_memory_structure() {
        std::map<std::string, Json::Value> structure;
        for (const auto& config : configurations) {
            std::string processorname = config["processname"].asString();
            structure[processorname] = config;
        }
        return structure;
    }

    // Function to get configuration by processor name
    Json::Value get_configuration(const std::string& processorname) const {
        auto it = config.find(processorname);
        if (it != config.end()) {
            return it->second;
        }
        return {};
    }

    // Function to get worker configurations by processor name
    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<int>, std::vector<std::string>, std::vector<std::string>> get_workers_config(const std::string& processorname) const {
        Json::Value config = get_configuration(processorname);
        if (!config.isNull()) {
            std::vector<std::string> result_socket_type, result_dataflow_type, result_lp_sockets, result_hp_sockets, workername, name_workers;
            std::vector<int> num_workers;

            for (const auto& manager : config["manager"]) {
                result_socket_type.push_back(manager["result_socket_type"].asString());
                result_dataflow_type.push_back(manager["result_dataflow_type"].asString());
                result_lp_sockets.push_back(manager["result_lp_socket"].asString());
                result_hp_sockets.push_back(manager["result_hp_socket"].asString());
                num_workers.push_back(manager["num_workers"].asInt());
                workername.push_back(manager["name"].asString());
                name_workers.push_back(manager["name_workers"].asString());
            }

            return { result_socket_type, result_dataflow_type, result_lp_sockets, result_hp_sockets, num_workers, workername, name_workers };
        }
        return { {}, {}, {}, {}, {}, {}, {} };
    }

private:
    std::vector<Json::Value> configurations;
    std::map<std::string, Json::Value> config;

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

    const std::vector<std::string> MANAGER_FIELDS = {
        "result_socket_type",
        "result_dataflow_type",
        "result_lp_socket",
        "result_hp_socket",
        "num_workers",
        "name",
        "name_workers"
    };

    // Function to validate configurations
    void validate_configurations(const std::vector<Json::Value>& configurations) const {
        for (const auto& config : configurations) {
            for (const auto& field : REQUIRED_FIELDS) {
                if (!config.isMember(field) || config[field].isNull()) {
                    throw std::runtime_error("Field '" + field + "' is missing or not well-formed in one or more configurations.");
                }
            }
            for (const auto& manager : config["manager"]) {
                for (const auto& field : MANAGER_FIELDS) {
                    if (!manager.isMember(field) || manager[field].isNull()) {
                        throw std::runtime_error("Field '" + field + "' is missing or not well-formed in one or more manager configurations.");
                    }
                }
            }
        }
    }
};

