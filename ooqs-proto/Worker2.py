# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from WorkerBase import WorkerBase

class Worker2(WorkerBase):
	def __init__(self):
		super().__init__()

	def process_data(self, data):

		print(data)