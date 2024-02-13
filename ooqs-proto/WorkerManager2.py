# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerManager import WorkerManager
from WorkerThread2 import WorkerThread2

import io
import threading
import sys

class WorkerManager2(WorkerManager):
	def __init__(self, manager_id, supervisor, name = "S11"):
		super().__init__(manager_id, supervisor, name)

	def start_worker_threads(self, num_threads):
		super().start_worker_threads(num_threads)
		#Worker threads
		for i in range(num_threads):
			thread = WorkerThread2(i, self, "Rate2")
			self.worker_threads.append(thread)
			thread.start()

	def start_worker_processes(self, num_processes):
		super().start_worker_processes(num_processes)
		try:
			raise ValueError("No process processing_type available for this manager")
		except Exception as e:
			# Handle any other unexpected exceptions
			print(f"ERROR: An unexpected error occurred: {e}")
			self.supervisor.command_shutdown()
			sys.exit(1)

			