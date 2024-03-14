# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import sys
import threading
from Supervisor1 import Supervisor1

def main(json_file_path, consumername):
    #try:
        supervisor_instance = Supervisor1(json_file_path, consumername)
        supervisor_instance.start()
    #except Exception as e:
    #    print(f"Error: {e}")

if __name__ == "__main__":
    # Check if a JSON file path is provided as a command-line argument
    if len(sys.argv) != 2:
        print("Usage: python script.py <json_file_path> ")
        sys.exit(1)

    # Get the JSON file path from the command-line arguments
    json_file_path = sys.argv[1]
    consumername = "RTADP1"

    # Call the main function with the provided JSON file path
    main(json_file_path, consumername)
    
