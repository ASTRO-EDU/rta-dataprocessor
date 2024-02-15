# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import avro.schema
from avro.io import DatumReader
import zmq
import io
import json
import struct
import sys
from ConfigurationManager import ConfigurationManager

def load_configuration(self, config_file, name="CONS1"):
    config_manager = ConfigurationManager(config_file)
    config=config_manager.get_configuration(name)
    return config

def main(config_file_path, processname):
    config = load_configuration(config_file_path, processname)

    # Connect to the ZeroMQ pull-type endpoint
    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.bind(config.get("data_lp_socket_pull"))  # Replace with your actual endpoint

    # Load Avro schema from the provided schema string
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

    # Create Avro reader
    reader = avro.io.DatumReader(avro_schema)

    # Receive and decode Avro messages continuously
    while True:
        avro_binary_data = socket.recv()

        # Deserialize the Avro message using avro library
        bytes_io = io.BytesIO(avro_binary_data)
        decoder = avro.io.BinaryDecoder(bytes_io)
        avro_message = reader.read(decoder)

        # Process the decoded Avro message as needed
        print(avro_message)

def read_config(file_path="config.json"):
    with open(file_path, "r") as file:
        config = json.load(file)
    return config

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py <config_file> <processname>")
        sys.exit(1)

    config_file_path = sys.argv[1]
    processname = sys.argv[2]

    main(config_file_path, processname)
