from Supervisor import Supervisor
from WorkerManager1 import WorkerManager1

class Supervisor1(Supervisor):
	def __init__(self, config_file="config.json", name="OOQS1"):
		super().__init__(config_file, name)

	def start_managers(self):
		indexmanager=0
		manager1 = WorkerManager1(self, "S22Rate", self.result_sockets[indexmanager], self.result_sockets_type[indexmanager], self.result_dataflow_type[indexmanager])
		manager1.start()
		self.manager_workers.append(manager1)

		indexmanager=1
		manager2 = WorkerManager1(self, "S22Mean", self.result_sockets[indexmanager], self.result_sockets_type[indexmanager], self.result_dataflow_type[indexmanager])
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
