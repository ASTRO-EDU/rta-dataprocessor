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

def publish_data(socketstring):
    context = zmq.Context()
    socket = context.socket(zmq.PUB)
    socket.bind(socketstring)

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



if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python ProcessMonitoring.py <socket>")
        sys.exit(1)

    socket = sys.argv[1]
    publish_data(socket)
