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
from avro_schema import AVRO_SCHEMA

class Worker1(WorkerBase):
	def __init__(self):
		super().__init__()

		# Load Avro schema from the provided schema string
		avro_schema = avro.schema.parse(AVRO_SCHEMA)
		# Create Avro reader
		self.reader = avro.io.DatumReader(avro_schema)

	def config(self, configuration):
		super().config(configuration)

	def process_data(self, data, priority):

		#TODO: handle priority
		print(f"data is of type: {type(data)} and the content is \n\n: {data}\n")
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

			


			
