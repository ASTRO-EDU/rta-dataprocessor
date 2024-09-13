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
from Command import Command

def main():
    if len(sys.argv) != 4:
        print("Usage: python script.py <config_file> <command_type> <pidtarget_processname>")
        sys.exit(1)

     
    config_file_path = sys.argv[1]


    command_subtype = sys.argv[2].lower()
    pidtarget_processname = sys.argv[3]

    if command_subtype not in ["shutdown", "cleanedshutdown", "start", "stop", "startprocessing", "stopprocessing", "reset", "startdata", "stopdata",  "getstatus"]:
        print("Invalid command type. Use 'shutdown', 'cleanedshutdown', 'start', 'stop', 'startprocessing', 'stopprocessing', 'reset', 'startdata', 'stopdata',  or 'getstatus'.")
        sys.exit(1)

    command = Command(config_file_path, "CommandCenter")  # Adjust the address based on your setup

    try:
        
        command.send_command(command_subtype, pidtarget_processname)

    except KeyboardInterrupt:
        print("Command generation stopped.")

if __name__ == "__main__":
    main()
