# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import sys
import threading
from Supervisor1 import Supervisor1

def main(json_file_path, consumername, dataflowtype):
    #try:
        supervisor_instance = Supervisor1(json_file_path, dataflowtype, consumername)
        supervisor_instance.start()
    #except Exception as e:
    #    print(f"Error: {e}")

if __name__ == "__main__":
    # Check if a JSON file path is provided as a command-line argument
    if len(sys.argv) != 4:
        print("Usage: python script.py <json_file_path> <consumername> <dataflowtype>. ")
        sys.exit(1)

    # Get the JSON file path from the command-line arguments
    json_file_path = sys.argv[1]
    consumername = sys.argv[2]
    dataflowtype = sys.argv[3]

    if dataflowtype not in ["Stream", "File", "String"]:
        print("Invalid command type. Use 'Stream', 'File', 'String'.")
        sys.exit(1)

    # Call the main function with the provided JSON file path
    main(json_file_path, consumername, dataflowtype)
    
