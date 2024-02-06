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
	def __init__(self, supervisor, manager_type="Thread", name = "S22"):
		super().__init__(supervisor, manager_type, name)

	def start_worker_threads(self, num_threads=5):
		#Worker threads
		for i in range(num_threads):
			thread = WorkerThread1(i, self, "Rate")
			self.worker_threads.append(thread)
			thread.start()

	def start_worker_processes(self, num_processes=5):
		# Worker processes
		for i in range(num_processes):
			process = WorkerProcess1(i, self, self.processdata_shared)
			self.worker_processes.append(process)
			process.start()
			