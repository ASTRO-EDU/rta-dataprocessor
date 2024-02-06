import avro.schema
from avro.io import DatumReader
import zmq
import io
import json
import struct
import sys

def main(config_file_path):
    config = read_config(config_file_path)

    # Connect to the ZeroMQ pull-type endpoint
    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.bind(config["datastream_lp_socket_pull"])  # Replace with your actual endpoint

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
    if len(sys.argv) != 2:
        print("Usage: python script.py <config_file>")
        sys.exit(1)

    config_file_path = sys.argv[1]

    main(config_file_path)
