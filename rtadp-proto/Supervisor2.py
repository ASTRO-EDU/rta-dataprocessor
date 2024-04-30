# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from Supervisor import Supervisor
from WorkerManager2 import WorkerManager2

class Supervisor2(Supervisor):
	def __init__(self, config_file="config.json", name="RTADP2"):
		super().__init__(config_file, name)

	def start_managers(self):
		indexmanager=0
		manager1 = WorkerManager2(indexmanager, self, "Rate")
		manager1.start()
		self.manager_workers.append(manager1)


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
