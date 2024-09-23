# Copyright (C) 2024 INAF
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
from ConfigurationManager import ConfigurationManager

class AvroDataGenerator:
    def __init__(self, config_file_path, queue, delay, processname):
        self.load_configuration(config_file_path, processname)
        self.delay = float(delay)
        self.queue = queue
        self.type = self.config.get("dataflow_type")
        self.datasocket_type = self.config.get("datasocket_type")
        print(f"{self.type} {self.datasocket_type}")

        self.context = zmq.Context()
        if self.config.get("datasocket_type") == "pushpull":
            self.socket = self.context.socket(zmq.PUSH)
        if self.config.get("datasocket_type") == "pubsub":
            self.socket = self.context.socket(zmq.PUB)

        print("Connecting hp to:" , self.config.get("data_hp_socket"))
        print("Connecting lp to:" , self.config.get("data_hp_socket"))

        if self.queue == "hp":
            if self.config.get("datasocket_type") == "pushpull":
                self.socket.connect(self.config.get("data_hp_socket"))
            if self.config.get("datasocket_type") == "pubsub":
                self.socket.bind(self.config.get("data_hp_socket"))
        else:
            if self.config.get("datasocket_type") == "pushpull":
                self.socket.connect(self.config.get("data_lp_socket"))
            if self.config.get("datasocket_type") == "pubsub":
                self.socket.bind(self.config.get("data_lp_socket"))

        #monitoring
        self.start_time = time.time()
        self.next_time = self.start_time
        self.processed_data_count = 0
        self.processing_rate = 0

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)

    def generate_avro_data(self):
        # Generate an array of X double values for the 'data' field
        avro_data = [random.uniform(0, 100) for _ in range(50)]
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
            "timestamp": float(time.time()),
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
        self.command_thread = threading.Thread(target=self.calcdatarate, daemon=True)
        self.command_thread.start()

        while True:
            avro_message, avro_schema = self.create_avro_message()
            self.processed_data_count += 1
            #print(avro_message)
            # Serialize the Avro message using avro library
            if self.type == "binary":
                writer = avro.io.DatumWriter(avro_schema)
                bytes_io = io.BytesIO()
                encoder = avro.io.BinaryEncoder(bytes_io)
                writer.write(avro_message, encoder)
                avro_binary_data = bytes_io.getvalue()

                # Send the serialized Avro message via ZeroMQ
                self.socket.send(avro_binary_data)
            
            if self.type == "string":
                self.socket.send_string(json.dumps(avro_message))

            # Simulate a delay (adjust as needed)
            time.sleep(self.delay)

    def calcdatarate(self):
        while True:
            update_interval=10
            time.sleep(1)        
            elapsed_time = time.time() - self.next_time
            if elapsed_time > update_interval:
                self.next_time = time.time()
                self.processing_rate = self.processed_data_count / elapsed_time
                print(self.processing_rate)
                self.processed_data_count = 0


if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python script.py <config_file> <queue (lp|hp)> <delay> <processnamedest>")
    
        sys.exit(1)

    config_file_path = sys.argv[1]
    queue = sys.argv[2]
    delay = sys.argv[3]
    processnamedest = sys.argv[4]

    avro_data_generator = AvroDataGenerator(config_file_path, queue, delay, processnamedest)
    avro_data_generator.main()

