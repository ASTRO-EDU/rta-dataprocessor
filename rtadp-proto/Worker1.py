# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerBase import WorkerBase
import avro.schema
from avro.io import DatumReader
import io

class Worker1(WorkerBase):
	def __init__(self):
		super().__init__()

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
		self.reader = avro.io.DatumReader(avro_schema)

	def process_data(self, data):

		# Custom processing logic for Worker1
		#print("Process data...")
		# Deserialize the Avro message using avro library
		if self.supervisor.dataflowtype == "binary":
			bytes_io = io.BytesIO(data)
			decoder = avro.io.BinaryDecoder(bytes_io)
			avro_message = self.reader.read(decoder)

			# Process the decoded Avro message as needed
			#print(self.globalname)
			#print(avro_message)
			#print(data)
			self.manager.result_queue.put(data)
			
		if self.supervisor.dataflowtype == "filename":
			print(data)
			self.manager.result_queue.put(str(data))

		if self.supervisor.dataflowtype == "string":
			#print(data)
			#self.manager.result_queue.put(data)
			#self.socket_result.send_string(data)
			#print("WorkerProcess1")
			self.manager.result_queue.put(str(data))
			


			
