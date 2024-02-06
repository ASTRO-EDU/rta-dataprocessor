# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import json
import time
import io
import random
import zmq
import threading
import avro.schema
import avro.io
import sys
import h5py
import os

class AvroDataGenerator:
    def __init__(self, config_file_path, queue, interval, output_folder="output"):
        self.config = self.read_config(config_file_path)
        self.interval = float(interval)
        self.queue = queue
        self.output_folder = output_folder

        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.PUSH)

        if self.queue == "hp":
            self.socket.connect(self.config["datafile_hp_socket_push"])
        else:
            self.socket.connect(self.config["datafile_lp_socket_push"])

        self.processed_data_count = 0
        self.processing_rate = 0

    def generate_avro_data(self):
        # Generate an array of 16384 double values for the 'data' field
        avro_data = [random.uniform(0, 100) for _ in range(16)]
        return avro_data

    def create_avro_message(self):
        avro_schema_str = '''
            {
                "type": "record",
                "name": "AvroMonitoringPoint",
                "namespace": "astri.mon.kafka",
                "fields": [
                    {"name": "assembly", "type": "string"},
                    {"name": "name", "type": "string"},
                    {"name": "serial_number", "type": "string"},
                    {"name": "timestamp", "type": "long"},
                    {"name": "source_timestamp", "type": ["null", "long"]},
                    {"name": "units", "type": "string"},
                    {"name": "archive_suppress", "type": "boolean"},
                    {"name": "env_id", "type": "string"},
                    {"name": "eng_gui", "type": "boolean"},
                    {"name": "op_gui", "type": "boolean"},
                    {"name": "data", "type": {"type": "array", "items": ["double", "int", "long", "string", "boolean"]}}
                ]
            }
        '''

        avro_schema = avro.schema.parse(avro_schema_str)

        # Fill in other fields based on your use case
        avro_message = {
            "assembly": "YourAssemblyName",
            "name": "YourDisplayName",
            "serial_number": "YourSerialNumber",
            "timestamp": int(time.time()),
            "source_timestamp": None,
            "units": "YourUnits",
            "archive_suppress": False,
            "env_id": "YourEnvID",
            "eng_gui": True,
            "op_gui": True,
            "data": self.generate_avro_data(),
        }

        return avro_message, avro_schema

    def main(self):
        self.monitoring_thread = threading.Thread(target=self.calcdatarate, daemon=True)
        self.monitoring_thread.start()

        start_time = time.time()
        current_file = None
        avro_messages = []

        while True:
            elapsed_time = time.time() - start_time

            if elapsed_time >= self.interval:
                start_time = time.time()
                current_file = self.create_hdf5_file()
                self.write_to_hdf5(current_file, avro_messages)
                avro_messages = []

            avro_message, avro_schema = self.create_avro_message()
            self.processed_data_count += 1
            avro_messages.append(avro_message)

    def read_config(self, file_path="config.json"):
        with open(file_path, "r") as file:
            config = json.load(file)
        return config

    def calcdatarate(self):
        while True:
            time.sleep(60)
            elapsed_time = time.time() - self.next_time
            if elapsed_time > 60:
                self.next_time = time.time()
                self.processing_rate = self.processed_data_count / elapsed_time
                print(self.processing_rate)
                self.processed_data_count = 0

    def create_hdf5_file(self):
        timestamp = int(time.time())
        filename = os.path.join(self.output_folder, f"data_{timestamp}.h5")

        # Create the directory if it does not exist
        os.makedirs(self.output_folder, exist_ok=True)
        
        return filename

    def write_to_hdf5(self, filename, avro_messages):
        with h5py.File(filename, "w") as file:
            for i, avro_message in enumerate(avro_messages):
                group = file.create_group(f"data_group_{i}")
                group.create_dataset("data", data=avro_message["data"])

        # Send the filename via ZeroMQ
        print(filename)
        self.socket.send(filename.encode())

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python script.py <config_file> <queue (lp|hp)> <interval>")
        sys.exit(1)

    config_file_path = sys.argv[1]
    queue = sys.argv[2]
    interval = sys.argv[3]

    avro_data_generator = AvroDataGenerator(config_file_path, queue, interval)
    avro_data_generator.main()

