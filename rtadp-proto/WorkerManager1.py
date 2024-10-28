# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerManager import WorkerManager
from WorkerProcess import WorkerProcess
from WorkerThread import WorkerThread
from Worker1 import Worker1

class WorkerManager1(WorkerManager):
	def __init__(self, manager_id, supervisor, name = ""):
		super().__init__(manager_id, supervisor, name)
		self.manager_id = manager_id

	def start_worker_threads(self, num_threads):
		super().start_worker_threads(num_threads)
		#Worker threads
		for i in range(num_threads):
			processor = Worker1()
			thread = WorkerThread(i, self, self.supervisor.name_workers[self.manager_id], processor, self._workers_stop_event)
			self.worker_threads.append(thread)
			thread.start()
			#TODO: start_worker_threads must be implemented in WorkerManager. Just function returning processor object must be defined here 

	def start_worker_processes(self, num_processes):
		super().start_worker_processes(num_processes)
		# Worker processes
		for i in range(num_processes):
			processor = Worker1()
			process = WorkerProcess(i, self, self.supervisor.name_workers[self.manager_id], processor, self._workers_stop_event)
			self.worker_processes.append(process)
			process.start()
			#TODO: start_worker_processes must be implemented in WorkerManager. Just function returning processor object must be defined here 