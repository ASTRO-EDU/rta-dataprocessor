# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerThread import WorkerThread
import avro.schema
from avro.io import DatumReader
import io

class WorkerThread2(WorkerThread):
	def __init__(self, thread_id, workermanager, name="S11Rate"):
		super().__init__(thread_id, workermanager, name)

	def process_data(self, data, priority):
		super().process_data(data, priority)

		print(data)