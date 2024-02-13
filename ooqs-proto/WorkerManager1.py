# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerManager import WorkerManager
from WorkerThread1 import WorkerThread1
from WorkerProcess1 import WorkerProcess1
import io
import threading

class WorkerManager1(WorkerManager):
	def __init__(self, manager_id, supervisor, name = "S22"):
		super().__init__(manager_id, supervisor, name)

	def start_worker_threads(self, num_threads):
		super().start_worker_threads(num_threads)
		#Worker threads
		for i in range(num_threads):
			thread = WorkerThread1(i, self, "Rate")
			self.worker_threads.append(thread)
			thread.start()

	def start_worker_processes(self, num_processes):
		super().start_worker_processes(num_processes)
		# Worker processes
		for i in range(num_processes):
			process = WorkerProcess1(i, self, self.processdata_shared, "Rate")
			self.worker_processes.append(process)
			process.start()
			