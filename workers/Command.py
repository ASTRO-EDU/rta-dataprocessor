# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
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

class Command:
    def __init__(self, command_monitoring_socket_bind, processname="Command"):
        self.context = zmq.Context()
        self.processname = processname

        # PUB socket for sending commands
        self.socket_command = self.context.socket(zmq.PUB)
        self.socket_command.bind(command_monitoring_socket_bind)
        print("Send commands to " + command_monitoring_socket_bind)

    def send_command(self, subtype, pidtarget_processname, priority="Low"):
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


def read_config(file_path="config.json"):
    with open(file_path, "r") as file:
        config = json.load(file)
    return config

def main():
    if len(sys.argv) != 4:
        print("Usage: python script.py <config_file> <command_type> <pidtarget_processname>")
        sys.exit(1)

     
    config_file_path = sys.argv[1]

    # Read configuration from the provided file
    config = read_config(config_file_path)

    command_subtype = sys.argv[2].lower()
    pidtarget_processname = sys.argv[3]

    if command_subtype not in ["shutdown", "cleanedshutdown", "start", "stop", "startdata", "stopdata",  "getstatus"]:
        print("Invalid command type. Use 'shutdown', 'cleanedshutdown', 'start', 'stop', 'startdata', 'stopdata',  or 'getstatus'.")
        sys.exit(1)

    command = Command(config["command_socket_pubsub"])  # Adjust the address based on your setup

    try:
        
        command.send_command(command_subtype, pidtarget_processname)

    except KeyboardInterrupt:
        print("Command generation stopped.")

if __name__ == "__main__":
    main()
