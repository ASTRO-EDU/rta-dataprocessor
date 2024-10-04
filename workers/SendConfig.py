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

    pidtarget_processname = sys.argv[2]

    config_json = sys.argv[3]
    
    command = Command(config_file_path, "CommandCenter")  # Adjust the address based on your setup

    try:
        
        command.send_configuration(pidtarget_processname,config_json)

    except KeyboardInterrupt:
        print("Command generation stopped.")

if __name__ == "__main__":
    main()
