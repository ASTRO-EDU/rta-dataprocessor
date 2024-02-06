# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import zmq
import json
import time
import sys

def read_config(file_path="config.json"):
    with open(file_path, "r") as file:
        config = json.load(file)
    return config

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python ProcessMonitoring.py <config_file>")
        sys.exit(1)

    config_file_path = sys.argv[1]

    # Read configuration from the provided file
    config = read_config(config_file_path)


    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.bind(config["monitoring_socket_pull"])

    while True:
        message = socket.recv_string()
        decoded_message = json.loads(message)
        # Esegui l'aggregazione o altre operazioni sui dati ricevuti
        print("Dati ricevuti:", decoded_message)

