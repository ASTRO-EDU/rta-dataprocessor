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

	def init(self, manager: WorkerManager, supervisor: Supervisor, globalname: str):
		self.manager = manager
		self.supervisor = supervisor
		self.logger = supervisor.logger
		self.globalname = globalname

		#config
		self.context = zmq.Context()
		self.socket_config = self.context.socket(zmq.SUB)
		self.socket_config.connect(self.supervisor.config.get("config_socket"))
		self.socket_config.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics

	#to be reimplemented ####
	def config(configuration):
		print(f"Received config: {config}")
		pass
	

	#to be reimplemented ####
	def process_data(self, data, priority):
		pass


