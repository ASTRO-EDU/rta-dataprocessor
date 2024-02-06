from MonitoringPoint import MonitoringPoint
from WorkerThread import WorkerThread
from MonitoringThread import MonitoringThread
from WorkerManager import WorkerManager
from WorkerProcess import WorkerProcess
import json
import zmq
import queue
import threading
# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
import time
import sys
import psutil

class Supervisor:
    #dataflowtype = Stream | File
    def __init__(self, config_file="config.json", dataflowtype="Stream", name = "None"):
        self.load_configuration(config_file)
        self.name = name
        self.globalname = "Supervisor-"+name
        self.continueall = True
        self.dataflowtype = dataflowtype

        self.pid = psutil.Process().pid

        self.context = zmq.Context()

        print(f"Supervisor: {self.dataflowtype} configuration")   
        if self.dataflowtype == "Stream":
            #low priority data stream connection
            self.socket_lp_data = self.context.socket(zmq.PULL)
            self.socket_lp_data.bind(self.config_data["datastream_lp_socket_pull"])
            #high priority data stream connection
            self.socket_hp_data = self.context.socket(zmq.PULL)
            self.socket_hp_data.bind(self.config_data["datastream_hp_socket_pull"])

        if self.dataflowtype == "File":
        #low priority data file connection
            self.socket_lp_file = self.context.socket(zmq.PULL)
            self.socket_lp_file.bind(self.config_data["datafile_lp_socket_pull"])
            #high priority data file connection
            self.socket_hp_file = self.context.socket(zmq.PULL)
            self.socket_hp_file.bind(self.config_data["datafile_hp_socket_pull"])
        
        self.socket_command = self.context.socket(zmq.SUB)
        self.socket_command.connect(self.config_data["command_socket_pubsub"])
        self.socket_command.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
        
        self.socket_monitoring = self.context.socket(zmq.PUSH)
        self.socket_monitoring.connect(self.config_data["monitoring_socket_push"])
        # self.monitoringpoint = MonitoringPoint(self)
        # self.monitoring_thread = None

        self.manager_workers = []
        self.status = "Initialised"

        #process data based on Supervisor state
        self.processdata = 0
        self.suspenddata = False

        print(f"{self.globalname} started")

    def load_configuration(self, config_file):
        try:
            with open(config_file, "r") as file:
                self.config_data = json.load(file)
                print(self.config_data)
        except FileNotFoundError:
            print(f"Error: File '{config_file}' not found.")
            return
        except json.JSONDecodeError:
            print(f"Error: Invalid JSON format in file '{config_file}'.")
            return

        self.num_workers = self.config_data.get("num_workers", 5)

    def start_service_threads(self):
        #Monitoring thread
        # self.monitoring_thread = MonitoringThread(self.socket_monitoring, self.monitoringpoint)
        # self.monitoring_thread.start()
        #Command receiving thread
        #self.command_thread = threading.Thread(target=self.listen_for_commands, daemon=True)
        #self.command_thread.start()

        if self.dataflowtype == "Stream":
            #Data receiving on two queues: high and low priority
            self.lp_data_thread = threading.Thread(target=self.listen_for_lp_data, daemon=True)
            self.lp_data_thread.start()

            self.hp_data_thread = threading.Thread(target=self.listen_for_hp_data, daemon=True)
            self.hp_data_thread.start()
        
        if self.dataflowtype == "File":
            #Data receiving on two queues: high and low priority
            self.lp_data_thread = threading.Thread(target=self.listen_for_lp_file, daemon=True)
            self.lp_data_thread.start()

            self.hp_data_thread = threading.Thread(target=self.listen_for_hp_file, daemon=True)
            self.hp_data_thread.start()       

    #to be reimplemented ####
    def start_managers(self):
        #manager_type="Process" or manager_type="Thread"
        manager = WorkerManager(self, "Process", "Generic")
        manager.start()
        self.manager_workers.append(manager)

    def start_workers(self, num_workers=5):
        for manager in self.manager_workers: 
            if manager.manager_type == "Thread":
                manager.start_worker_threads(num_workers)
            if manager.manager_type == "Process":
                manager.start_worker_processes(num_workers)

    def start(self):
        self.start_service_threads()
        self.start_managers()
        self.start_workers(self.num_workers)

        self.status = "Waiting"

        try:
            while self.continueall:
                self.listen_for_commands()
                time.sleep(1)  # To avoid 100 per cent CPU
        except KeyboardInterrupt:
            print("Keyboard interrupt received. Terminating.")
            self.stop_all()
            self.continueall = False

    def listen_for_lp_data(self):
        while True:
            if not self.suspenddata:
                data = self.socket_lp_data.recv()
                for manager in self.manager_workers: 
                    manager.low_priority_queue.put(data) 

    def listen_for_hp_data(self):
        while True:
            if not self.suspenddata:
                data = self.socket_hp_data.recv()
                for manager in self.manager_workers: 
                    self.high_priority_queue.put(data) 

    def listen_for_lp_file(self):
        while True:
            if not self.suspenddata:
                filename = self.socket_lp_file.recv()
                for manager in self.manager_workers: 
                    manager.low_priority_queue.put(filename) 

    def listen_for_hp_file(self):
        while True:
            if not self.suspenddata:
                filename = self.socket_hp_file.recv()
                for manager in self.manager_workers: 
                    self.high_priority_queue.put(filename) 

    def listen_for_commands(self):
        while True:
            print("Waiting for commands...")
            command = json.loads(self.socket_command.recv_string())
            self.process_command(command)

    def process_command(self, command):
        print(f"Received command: {command}")
        subtype_value = command['header']['subtype']
        pidtarget = command['header']['pidtarget']
        pidsource = command['header']['pidsource']
        if pidtarget == self.name or pidtarget == "all".lower() or pidtarget == "*":
            if subtype_value == "shutdown":
                self.status = "Shutdown"
                self.stop_all(True)
                self.continueall = False
            if subtype_value == "cleanedshutdown":
                if self.status == "Processing":
                    self.status = "EndingProcessing"
                    self.suspenddata = True
                    for manager in self.manager_workers:
                        print(f"Trying to stop {manager.globalname}...")
                        manager.status = "EndingProcessing"
                        manager.suspenddata = True
                        while manager.low_priority_queue.qsize() != 0 and manager.low_priority_queue.qsize() != 0:
                            time.sleep(0.1)
                        print(f"Queues of manager {manager.globalname} are empty {manager.low_priority_queue.qsize()} {manager.low_priority_queue.qsize() }")
                        manager.status = "Shutdown"
                else:
                    print("WARNING! Not in Processing state for a cleaned shutdown. Force the shutdown.") 
                self.status = "Shutdown"
                self.stop_all(False)
                self.continueall = False
            if subtype_value == "getstatus":
                for manager in self.manager_workers:
                    manager.monitoring_thread.sendto(pidsource)
            if subtype_value == "start": #data processing
                self.status = "Processing"
                for manager in self.manager_workers:
                    manager.status = "Processing"
                    manager.set_processdata(1)
                pass
            if subtype_value == "suspend": #data processing
                self.status = "Suspend"
                for manager in self.manager_workers:
                    manager.status = "Suspend"
                    manager.set_processdata(0)
                pass
            if subtype_value == "suspenddata": #data acquisition
                for manager in self.manager_workers:
                    manager.suspenddata = True
                pass
            if subtype_value == "startdata": #data acquisition
                for manager in self.manager_workers:
                    manager.suspenddata = False
                pass
            if subtype_value == "restart": #data processing
                self.status = "Processing"
                for manager in self.manager_workers:
                    manager.status = "Processing"
                    manager.set_processdata(1)
                pass       
            if subtype_value == "stop": #data processing
                self.status = "Waiting"
                for manager in self.manager_workers:
                    manager.status = "Waiting"
                    manager.set_processdata(0)
                pass    
        # monitoringpoint_data = self.monitoringpoint.get_data()
        # print(f"MonitoringPoint data: {monitoringpoint_data}")

    def stop_all(self, fast=False):
        print("Stopping all workers and managers...")
        # Stop monitoring thread
        # self.monitoring_thread.stop()
        # self.monitoring_thread.join()

        self.suspenddata = True
        time.sleep(0.1)

        # Stop worker threads
        for manager in self.manager_workers: 
            for thread in manager.worker_threads:
                thread.stop()
                thread.join()


       # Stop worker processes
        for manager in self.manager_workers: 
            for process in manager.worker_processes:
                process.stop()
                process.join()

        # Stop managers
        for manager in self.manager_workers: 
            manager.stop(fast)
            manager.join()

        print("All workers and managers terminated.")
        sys.exit(0)

