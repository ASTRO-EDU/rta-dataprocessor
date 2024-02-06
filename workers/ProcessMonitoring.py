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
import sys

class MonitoringConsumer:
    def __init__(self, monitoring_socket_bind):
        self.context = zmq.Context()
        self.socket_monitoring = self.context.socket(zmq.PULL)
        self.socket_monitoring.bind(monitoring_socket_bind)

    def receive_and_decode_messages(self):
        while True:
            message = self.socket_monitoring.recv_string()
            decoded_message = json.loads(message)
            print("Received and decoded message:", decoded_message)

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

    # Use the configuration to initialize the MonitoringConsumer
    monitoring_consumer = MonitoringConsumer(config["monitoring_socket_pull"])

    try:
        monitoring_consumer.receive_and_decode_messages()
    except KeyboardInterrupt:
        print("Consumer stopped.")
