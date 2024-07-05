# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import sys
import zmq
import json
import time
from ConfigurationManager import ConfigurationManager

class WorkerConfig:
    def __init__(self, config_file_path, processname="CommandCenter"):
        self.context = zmq.Context()
        self.processname = processname
        self.load_configuration(config_file_path, processname)

        # PUB socket for sending commands
        self.socket_config = self.context.socket(zmq.PUB)
        self.socket_config.bind(self.config.get("config_socket"))
        print("Send commands to " + self.config.get("config_socket"))

    def send_config(self, workerconfigfile, wname, priority="Low"):
        time.sleep(0.3) #wait otherwise the first message is not received
        header = {
            "type": 3,
            #"time": time.strftime("%Y-%m-%dT%H:%M:%SZ"),
			"time": time.time(),
            "pidsource": self.processname,
            "pidtarget": wname,
            "priority": priority
        }

        try:
            with open(workerconfigfile, "r") as file:
                wconfig = json.load(file)
                print(wconfig)

        except FileNotFoundError:
            print(f"Error: File '{workerconfigfile}' not found.")
            return
        except json.JSONDecodeError:
            print(f"Error: Invalid JSON format in file '{workerconfigfile}'.")
            return


        configworker = {"header": header, "config": wconfig}
        message = json.dumps(configworker)
        self.socket_config.send_string(message)
        print(f"Sent to {wname} config." + message)

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)



def main():
    if len(sys.argv) != 4:
        print("Usage: python SendWorkerConfig.py <config_file> <workerconfig.json> <WorkersName or WorkerName>")
        sys.exit(1)

     
    config_file_path = sys.argv[1]


    workerconfigfile = sys.argv[2]
    wname = sys.argv[3]


    wconfig = WorkerConfig(config_file_path, "CommandCenter")  # Adjust the address based on your setup

    try:
        
        wconfig.send_config(workerconfigfile, wname)

    except KeyboardInterrupt:
        print("Worker Config generation stopped.")

if __name__ == "__main__":
    main()
