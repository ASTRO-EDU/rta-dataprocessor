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
import time
import random

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

	def config(self, configuration):
		super().config(configuration)


	def process_data(self, data):

		# Custom processing logic for Worker1
		#print("Process data...")
		# Deserialize the Avro message using avro library
		if self.supervisor.dataflowtype == "binary":
			bytes_io = io.BytesIO(data)
			decoder = avro.io.BinaryDecoder(bytes_io)
			avro_message = self.reader.read(decoder)
			time.sleep(random.uniform(0, 0.1)) #simulate processing

			# Process the decoded Avro message as needed
			#print(self.globalname)
			#print(avro_message)
			#print(data)
			return data
			
		if self.supervisor.dataflowtype == "filename":
			time.sleep(random.uniform(0, 0.1)) #simulate processing
			print(data)
			return str(data)

		if self.supervisor.dataflowtype == "string":
			#time.sleep(random.uniform(0, 0.1)) #simulate processing
			return str(data)

		#example of generation of alarm, log, info
		#self.supervisor.send_alarm(0, "alarm", self.fullname)
		#self.supervisor.send_log(0, "log message", self.fullname)
		#self.supervisor.send_info(0, "information message", self.fullname)

			


			
