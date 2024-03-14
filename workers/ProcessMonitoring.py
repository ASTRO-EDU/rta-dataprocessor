# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import zmq
import json
import sys
from ConfigurationManager import ConfigurationManager

class MonitoringConsumer:
    def __init__(self, config_file_path, processname="CommandCenter"):
        self.processname = processname
        self.load_configuration(config_file_path, processname)
        self.context = zmq.Context()
        self.socket_monitoring = self.context.socket(zmq.PULL)
        self.socket_monitoring.bind(self.config.get("monitoring_socket"))

    def receive_and_decode_messages(self):
        while True:
            message = self.socket_monitoring.recv_string()
            decoded_message = json.loads(message)
            print("Received and decoded message:", decoded_message)

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python ProcessMonitoring.py <config_file>")
        sys.exit(1)

    config_file_path = sys.argv[1]

    # Use the configuration to initialize the MonitoringConsumer
    monitoring_consumer = MonitoringConsumer(config_file_path, "CommandCenter")

    try:
        monitoring_consumer.receive_and_decode_messages()
    except KeyboardInterrupt:
        print("Consumer stopped.")
