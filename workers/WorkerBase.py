# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#

import json
import zmq
from logging import Logger
from Supervisor import Supervisor
from WorkerManager import WorkerManager

class WorkerBase():
	def __init__(self):
		pass
	
	def init(self, manager: WorkerManager, supervisor: Supervisor, workersname: str, fullname: str):
		self.manager = manager
		self.supervisor = supervisor
		self.logger = supervisor.logger
		self.workersname = workersname
		self.fullname = fullname

	#to be reimplemented ####
	def config(self, conf_message):
		# Get configuration
		configuration = conf_message['config']['config']
		print(f"Received config: {configuration}")
		self.logger.system(f"Received config: {configuration}")
		return configuration


	

	#to be reimplemented ####
	def process_data(self, data, priority):
		pass

