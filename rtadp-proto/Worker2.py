# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerBase import WorkerBase
import time

class Worker2(WorkerBase):
	def __init__(self):
		super().__init__()

	def process_data(self, data):
		time.sleep(0.1)
		return None
