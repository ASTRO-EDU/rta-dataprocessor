
import json

def get_pull_config(address):
    # Split the string based on the colon
    parts = address.split(":")

    if len(parts) == 3 and parts[0] == "tcp":
        # Reconstruct the desired string
        return f"{parts[0]}://*:{parts[2]}"

class ConfigurationManager:
    REQUIRED_FIELDS = [
        "processname",
        "dataflow_type",
        "processing_type",
        "datasocket_type",
        "data_lp_socket",
        "data_hp_socket",
        "command_socket",
        "monitoring_socket",
        "manager_result_socket",
        "manager_result_socket_type",
        "manager_num_workers",
        "comment"
    ]

    def __init__(self, file_path):
        self.configurations = self.read_configurations_from_file(file_path)
        self.config = self.create_memory_structure()

    def read_configurations_from_file(self, file_path):
        try:
            with open(file_path, "r") as file:
                configurations = json.load(file)
                #print(configurations)
            #self.validate_configurations(configurations)
            return configurations
        except FileNotFoundError:
            print(f"Error: File '{config_file}' not found.")
            return
        except json.JSONDecodeError:
            print(f"Error: Invalid JSON format in file '{config_file}'.")
            return

    def validate_configurations(self, configurations):
        for config in configurations:
            for field in self.REQUIRED_FIELDS:
                if field not in config or not config[field]:
                    raise ValueError(f"Field '{field}' is missing or not well-formed in one or more configurations.")

    def create_memory_structure(self):
        structure = {}
        for config in self.configurations:
            processorname = config["processname"]
            structure[processorname] = config
        return structure

    def get_configuration(self, processorname):
        return self.config.get(processorname)

    def get_workers_config(self, processorname):
        config = self.get_configuration(processorname)
        if config:
            result_socket_type = config.get("manager_result_socket_type", [])
            result_sockets = config.get("manager_result_socket", [])
            num_workers = config.get("manager_num_workers", [])
            return result_socket_type, result_sockets, num_workers
        else:
            return [], [], []


#config_manager = ConfigurationManager(file_path)
#ooqs1config=config_manager.get_configuration("OOQS1")
#ooqs1config.get("data_hp_socket")
#config_manager.get_pull_config("OOQS1", "data_hp_socket")


