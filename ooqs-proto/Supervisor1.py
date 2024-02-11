from Supervisor import Supervisor
from WorkerManager1 import WorkerManager1

class Supervisor1(Supervisor):
	def __init__(self, config_file="config.json", name="OOQS1"):
		super().__init__(config_file, name)

	def start_managers(self):
		manager1 = WorkerManager1(self, "S22Rate")
		manager1.start()
		self.manager_workers.append(manager1)

		# manager2 = WorkerManager1(self, "S22Mean")
		# manager2.start()
		# self.manager_workers.append(manager2)

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
