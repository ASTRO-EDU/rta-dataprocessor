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

def publish_data(config):
    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.bind(config["command_socket_pubsub"])

    data_sequence = [
        {"parameter1": 1, "parameter2": 2, "status": "Running"},
        {"parameter1": 3, "parameter2": 4, "status": "Paused"},
        {"parameter1": 5, "parameter2": 6, "status": "Stopped"}
    ]
    time.sleep(1)
    for data in data_sequence:
        message = json.dumps(data)
        socket.send_string(message)
        print(f"Sent: {message}")
        time.sleep(1)

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

    publish_data(config)
