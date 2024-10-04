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

class Command:
    def __init__(self, config_file_path, processname="CommandCenter"):
        self.context = zmq.Context()
        self.processname = processname
        self.load_configuration(config_file_path, processname)

        # PUB socket for sending commands
        self.socket_command = self.context.socket(zmq.PUB)
        self.socket_command.bind(self.config.get("command_socket"))
        print("Send commands to " + self.config.get("command_socket"))

    def send_command(self,subtype, pidtarget_processname, priority="Low"):
        time.sleep(0.3) #wait otherwise the first message is not received
        header = {
            "type": 0,
            "subtype": subtype,
            #"time": time.strftime("%Y-%m-%dT%H:%M:%SZ"),
			"time": time.time(),
            "pidsource": self.processname,
            "pidtarget": pidtarget_processname,
            "priority": priority
        }

        command = {"header": header}
        message = json.dumps(command)
        self.socket_command.send_string(message)
        print(f"Sent {subtype} command." + message)
        
    def send_configuration(self, pidtarget_processname, config_dictionary,priority="Low"):
        time.sleep(0.3) #wait otherwise the first message is not received
        header = {
            "type": 3,
            "subtype": "config",
            #"time": time.strftime("%Y-%m-%dT%H:%M:%SZ"),
			"time": time.time(),
            "pidsource": self.processname,
            "pidtarget": pidtarget_processname,
            "priority": priority
        }
        config = {
            "config":config_dictionary
        }

        command = {"header": header,'config':config}
        message = json.dumps(command)
        self.socket_command.send_string(message)
        print(f"Sent config command." + message)

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        
    def release_socket(self):
        self.socket_command.close()
        self.context.term()
        


