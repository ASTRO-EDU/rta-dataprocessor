# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from Supervisor import Supervisor
from WorkerManager1 import WorkerManager1
import zmq
import time
import json

class Supervisor1(Supervisor):
	def __init__(self, config_file="config.json", name="RTADP1"):
		super().__init__(config_file, name)

		#test
		# self.socket_result = self.context.socket(zmq.PUB)
		# self.socket_result.bind("tcp://localhost:5563")
		# self.socket_result = self.context.socket(zmq.PUSH)
		# self.socket_result.connect("tcp://localhost:5563")
		# data_sequence = [
		# 	{"parameter1": 1, "parameter2": 2, "status": "Running"},
		# 	{"parameter1": 3, "parameter2": 4, "status": "Paused"},
		# 	{"parameter1": 5, "parameter2": 6, "status": "Stopped"}
		# ]
		# time.sleep(1)
		# for data in data_sequence:
		# 	message = json.dumps(data)
		# 	self.socket_result.send_string(message)
		# 	print(f"Sent: {message}")
		# 	time.sleep(1)

	def start_managers(self):
		indexmanager=0
		manager1 = WorkerManager1(indexmanager, self, self.workername[indexmanager])
		self.setup_result_channel(manager1, indexmanager)
		manager1.start()
		self.manager_workers.append(manager1)

		indexmanager=1
		manager2 = WorkerManager1(indexmanager, self, "S22Mean")
		self.setup_result_channel(manager2, indexmanager)
		manager2.start()
		self.manager_workers.append(manager2)

	#to be reimplemented ####
	#Decode the data before load it into the queue. For "dataflowtype": "binary"
	def decode_data(self, data):
		return data

	#to be reimplemented ####
	#Open the file before load it into the queue. For "dataflowtype": "file"
	#Return an array of data and the size of the array
	def open_file(self, filename):
		f = [filename]
		return f, 1
