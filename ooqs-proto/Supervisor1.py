from Supervisor import Supervisor
from WorkerManager1 import WorkerManager1

class Supervisor1(Supervisor):
	def __init__(self, config_file="config.json", name="OOQS1"):
		super().__init__(config_file, name)

	def start_managers(self):
		manager1 = WorkerManager1(self, "S22Rate")
		manager1.start()
		self.manager_workers.append(manager1)

		# manager2 = WorkerManager1(self, "Thread", "S22Mean")
		# manager2.start()
		# self.manager_workers.append(manager2)

	def decode_file(self, filename):
		return filename
