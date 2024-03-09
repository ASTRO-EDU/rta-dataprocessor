# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#

import json

class WorkerBase():
	def __init__(self):
		pass

	def init(self, manager, supervisor):
		self.manager = manager
		self.supervisor = supervisor

	#to be reimplemented ####
	def process_data(self, data, priority):
		pass


