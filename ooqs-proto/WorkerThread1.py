# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerThread import WorkerThread
import avro.schema
from avro.io import DatumReader
import io

class WorkerThread1(WorkerThread):
	def __init__(self, thread_id, workermanager, name="S22Rate"):
		super().__init__(thread_id, workermanager, name)

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

	def process_data(self, data, priority):
		super().process_data(data, priority)
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
			self.manager.result_queue.put(data)
			
		if self.supervisor.dataflowtype == "filename":
			print(data)
			self.manager.result_queue.put(data)

		if self.supervisor.dataflowtype == "string":
			#print(data)
			self.manager.result_queue.put(data)
