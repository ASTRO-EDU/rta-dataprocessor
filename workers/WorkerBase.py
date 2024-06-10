# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#

import json
from logging import Logger
from Supervisor import Supervisor
from WorkerManager import WorkerManager

class WorkerBase():
	def __init__(self):
		pass

	def init(self, manager: WorkerManager, supervisor: Supervisor, globalname: str):
		self.manager = manager
		self.supervisor = supervisor
		self.logger = supervisor.logger
		self.globalname = globalname

	#to be reimplemented ####
	def process_data(self, data, priority):
		pass


