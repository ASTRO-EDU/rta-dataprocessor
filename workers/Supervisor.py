# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from MonitoringPoint import MonitoringPoint
from WorkerThread import WorkerThread
from MonitoringThread import MonitoringThread
from WorkerManager import WorkerManager
from WorkerProcess import WorkerProcess
from ConfigurationManager import ConfigurationManager, get_pull_config
import zmq
import json
import queue
import threading
import signal
import time
import sys
import psutil

class Supervisor:
    def __init__(self, config_file="config.json", name = "None"):
        self.name = name
        self.config_manager = None

        #workers manager config
        self.manager_num_workers = None
        self.manager_result_sockets = None 
        self.manager_result_sockets_type = None
        self.manager_result_dataflow_type = None

        self.load_configuration(config_file, name)
        self.globalname = "Supervisor-"+name
        self.continueall = True
        self.pid = psutil.Process().pid

        self.context = zmq.Context()

        try:
            self.processingtype = self.config.get("processing_type")
            self.dataflowtype = self.config.get("dataflow_type")
            self.datasockettype = self.config.get("datasocket_type")
     
            print(f"Supervisor: {self.globalname} / {self.dataflowtype} / {self.processingtype} / {self.datasockettype}")   

            if self.datasockettype == "pushpull":
                #low priority data stream connection
                self.socket_lp_data = self.context.socket(zmq.PULL)
                self.socket_lp_data.bind(get_pull_config(self.config.get("data_lp_socket")))
                #high priority data stream connection
                self.socket_hp_data = self.context.socket(zmq.PULL)
                self.socket_hp_data.bind(get_pull_config(self.config.get("data_hp_socket")))
            elif self.datasockettype == "pubsub":
                self.socket_lp_data = self.context.socket(zmq.SUB)
                self.socket_lp_data.connect(self.config.get("data_lp_socket"))
                self.socket_lp_data.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
                self.socket_hp_data = self.context.socket(zmq.SUB)
                self.socket_hp_data.connect(self.config.get("data_hp_socket"))
                self.socket_hp_data.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
            else:
                raise ValueError("Config file: datasockettype must be pushpull or pubsub")
            
            #command
            self.socket_command = self.context.socket(zmq.SUB)
            self.socket_command.connect(self.config.get("command_socket"))
            self.socket_command.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
            
            #monitoring
            self.socket_monitoring = self.context.socket(zmq.PUSH)
            self.socket_monitoring.connect(self.config.get("monitoring_socket"))
            # self.monitoringpoint = MonitoringPoint(self)
            # self.monitoring_thread = None

            #results
            self.socket_result = [None] * 100

        except Exception as e:
           # Handle any other unexpected exceptions
           print(f"ERROR: An unexpected error occurred: {e}")
           sys.exit(1)

        else:

            self.manager_workers = []

            #process data based on Supervisor state
            self.processdata = 0
            self.stopdata = False

            # Set up signal handlers
            signal.signal(signal.SIGTERM, self.handle_signals)
            signal.signal(signal.SIGINT, self.handle_signals)

            self.status = "Initialised"

            print(f"{self.globalname} started")

    def load_configuration(self, config_file, name):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        self.manager_result_sockets_type, self.manager_result_dataflow_type, self.manager_result_sockets, self.manager_num_workers = self.config_manager.get_workers_config(name)

    def start_service_threads(self):
        #Monitoring thread
        # self.monitoring_thread = MonitoringThread(self.socket_monitoring, self.monitoringpoint)
        # self.monitoring_thread.start()
        #Command receiving thread
        #self.command_thread = threading.Thread(target=self.listen_for_commands, daemon=True)
        #self.command_thread.start()

        if self.dataflowtype == "binary":
            #Data receiving on two queues: high and low priority
            self.lp_data_thread = threading.Thread(target=self.listen_for_lp_data, daemon=True)
            self.lp_data_thread.start()

            self.hp_data_thread = threading.Thread(target=self.listen_for_hp_data, daemon=True)
            self.hp_data_thread.start()
        
        if self.dataflowtype == "filename":
            #Data receiving on two queues: high and low priority
            self.lp_data_thread = threading.Thread(target=self.listen_for_lp_file, daemon=True)
            self.lp_data_thread.start()

            self.hp_data_thread = threading.Thread(target=self.listen_for_hp_file, daemon=True)
            self.hp_data_thread.start()

        if self.dataflowtype == "string":
            #Data receiving on two queues: high and low priority
            self.lp_data_thread = threading.Thread(target=self.listen_for_lp_string, daemon=True)
            self.lp_data_thread.start()

            self.hp_data_thread = threading.Thread(target=self.listen_for_hp_string, daemon=True)
            self.hp_data_thread.start()  

        self.result_thread = threading.Thread(target=self.listen_for_result, daemon=True)
        self.result_thread.start()       

    def setup_result_channel(self, manager, indexmanager):
        #output sockert
        self.socket_result[indexmanager] = None
        self.context = zmq.Context()

        if manager.result_socket != "none":
            if manager.result_socket_type == "pushpull":
                self.socket_result[indexmanager] = self.context.socket(zmq.PUSH)
                self.socket_result[indexmanager].connect(manager.result_socket)
                print(f"---result socket pushpull {manager.globalname} {manager.result_socket}")

            if manager.result_socket_type == "pubsub":
                self.socket_result[indexmanager] = self.context.socket(zmq.PUB)
                self.socket_result[indexmanager].bind(manager.result_socket)
                print(f"---result socket pushpull {manager.globalname} {manager.result_socket}")


    #to be reimplemented ####
    def start_managers(self):
        #PATTERN
        indexmanager=0
        manager = WorkerManager(indexmanager, self, "Generic")
        self.setup_result_channel(manager, indexmanager)
        manager.start()
        self.manager_workers.append(manager)

    def start_workers(self):
        indexmanager=0
        for manager in self.manager_workers: 
            if self.processingtype == "thread":
                manager.start_worker_threads(self.manager_num_workers[indexmanager])
            if self.processingtype == "process":
                manager.start_worker_processes(self.manager_num_workers[indexmanager])
            indexmanager = indexmanager + 1

    def start(self):
        self.start_service_threads()
        self.start_managers()
        self.start_workers()

        self.status = "Waiting"

        try:
            while self.continueall:
                self.listen_for_commands()
                time.sleep(1)  # To avoid 100 per cent CPU
        except KeyboardInterrupt:
            print("Keyboard interrupt received. Terminating.")
            self.command_shutdown()

    def handle_signals(self, signum, frame):
        # Handle different signals
        if signum == signal.SIGTERM:
            print("SIGTERM received. Terminating with cleanedshutdown.")
            self.command_cleanedshutdown()
        elif signum == signal.SIGINT:
            print("SIGINT received. Terminating with shutdown.")
            self.command_shutdown()
        else:
            print(f"Received signal {signum}. Terminating.")
            self.command_shutdown()

    #to be reimplemented ####
    #Decode the data before load it into the queue. For "dataflowtype": "binary"
    def decode_data(self, data):
        return data

    def listen_for_result(self):
        while self.continueall:
            time.sleep(0.001)
            indexmanager = 0
            for manager in self.manager_workers:
                self.send_result(manager, indexmanager) 
                indexmanager = indexmanager + 1

    def send_result(self, manager, indexmanager):
        data = None
        try:
            data = manager.result_queue.get_nowait()
        except queue.Empty:
            return
        except Exception as e:
            # Handle any other unexpected exceptions
            #print(f"WARNING: {e}")
            return

        if manager.result_socket == "none":
            #print("WARNING: no socket result available to send results")
            return
        if manager.result_dataflow_type == "string" or manager.result_dataflow_type == "filename":
            try:
                data = str(data)
                #print(manager.result_queue.qsize())
                self.socket_result[indexmanager].send_string(data)
            except Exception as e:
                # Handle any other unexpected exceptions
                print(f"ERROR: data not in string format to be send to : {e}")
        if manager.result_dataflow_type == "binary":
            try:
                data = str(manager.result_queue.get_nowait())
                self.socket_result[indexmanager].send(data)
                return 
            except Exception as e:
                # Handle any other unexpected exceptions
                print(f"ERROR: data not in binary format to be send to socket_result: {e}")

    def listen_for_lp_data(self):
        while True:
            if not self.stopdata:
                data = self.socket_lp_data.recv()
                for manager in self.manager_workers:
                    decodeddata = self.decode_data(data)  
                    manager.low_priority_queue.put(decodeddata) 

    def listen_for_hp_data(self):
        while True:
            if not self.stopdata:
                data = self.socket_hp_data.recv()
                for manager in self.manager_workers: 
                    decodeddata = self.decode_data(data)
                    manager.high_priority_queue.put(decodeddata) 

    def listen_for_lp_string(self):
        while True:
            if not self.stopdata:
                data = self.socket_lp_data.recv_string()
                for manager in self.manager_workers: 
                    manager.low_priority_queue.put(data) 

    def listen_for_hp_string(self):
        while True:
            if not self.stopdata:
                data = self.socket_hp_data.recv_string()
                for manager in self.manager_workers: 
                    manager.high_priority_queue.put(data) 

    #to be reimplemented ####
    #Open the file before load it into the queue. For "dataflowtype": "file"
    #Return an array of data and the size of the array
    def open_file(self, filename):
        f = [filename]
        return f, 1

    def listen_for_lp_file(self):
        while True:
            if not self.stopdata:
                filename = self.socket_lp_data.recv()
                for manager in self.manager_workers: 
                    data, size = self.open_file(filename) 
                    for i in range(size):
                        manager.low_priority_queue.put(data[i]) 

    def listen_for_hp_file(self):
        while True:
            if not self.stopdata:
                filename = self.socket_hp_data.recv()
                for manager in self.manager_workers:
                    data, size = self.open_file(filename) 
                    for i in range(size):
                        manager.high_priority_queue.put(data[i]) 

    def listen_for_commands(self):
        while True:
            print("Waiting for commands...")
            command = json.loads(self.socket_command.recv_string())
            self.process_command(command)

    def command_shutdown(self):
        self.status = "Shutdown"
        self.command_stopdata()
        self.command_stop()
        self.stop_all(True)
        self.continueall = False
    
    def command_cleanedshutdown(self):
        if self.status == "Processing":
            self.status = "EndingProcessing"
            self.command_stopdata()
            for manager in self.manager_workers:
                print(f"Trying to stop {manager.globalname}...")
                manager.status = "EndingProcessing"
                while manager.low_priority_queue.qsize() != 0 and manager.low_priority_queue.qsize() != 0:
                    time.sleep(0.1)
                print(f"Queues data of manager {manager.globalname} have size {manager.low_priority_queue.qsize()} {manager.low_priority_queue.qsize()}")
                while manager.result_queue.qsize() != 0:
                    time.sleep(0.1) 
                print(f"Queues result of manager {manager.globalname} have size {manager.result_queue.qsize()}")
                manager.status = "Shutdown"
        else:
            print("WARNING! Not in Processing state for a cleaned shutdown. Force the shutdown.") 
        self.command_stop()
        self.stop_all(False)
        self.continueall = False
        self.status = "Shutdown"

    def command_reset(self):
        if self.status == "Processing" or self.status == "Waiting":
            self.command_stopdata()
            self.command_stop()
            for manager in self.manager_workers:
                print(f"Trying to reset {manager.globalname}...")
                manager.clean_queue()
                print(f"Queues of manager {manager.globalname} have size {manager.low_priority_queue.qsize()} {manager.low_priority_queue.qsize()} {manager.result_queue.qsize()}")
            self.status = "Waiting"


    def command_start(self):
        self.status = "Processing"
        for manager in self.manager_workers:
            manager.status = "Processing"
            manager.set_processdata(1)

    def command_stop(self):
        self.status = "Waiting"
        for manager in self.manager_workers:
            manager.status = "Waiting"
            manager.set_processdata(0)

    def command_startdata(self):
        self.stopdata = False
        for manager in self.manager_workers:
            manager.stopdata = False

    def command_stopdata(self):
        self.stopdata = True
        for manager in self.manager_workers:
            manager.stopdata = True

    def process_command(self, command):
        print(f"Received command: {command}")
        subtype_value = command['header']['subtype']
        pidtarget = command['header']['pidtarget']
        pidsource = command['header']['pidsource']
        if pidtarget == self.name or pidtarget == "all".lower() or pidtarget == "*":
            if subtype_value == "shutdown":
                self.command_shutdown()  
            if subtype_value == "cleanedshutdown":
                self.command_cleanedshutdown()
            if subtype_value == "getstatus":
                for manager in self.manager_workers:
                    manager.monitoring_thread.sendto(pidsource)
            if subtype_value == "start": #data processing
                    self.command_start()
            if subtype_value == "stop": #data processing
                    self.command_stop()
            if subtype_value == "reset": #reset the data processor
                    self.command_reset()
            if subtype_value == "stopdata": #data acquisition
                    self.command_stopdata()
            if subtype_value == "startdata": #data acquisition
                    self.command_startdata()
  
        # monitoringpoint_data = self.monitoringpoint.get_data()
        # print(f"MonitoringPoint data: {monitoringpoint_data}")

    def stop_all(self, fast=False):
        print("Stopping all workers and managers...")
        # Stop monitoring thread
        # self.monitoring_thread.stop()
        # self.monitoring_thread.join()

        self.command_stopdata()
        self.command_stop()
        time.sleep(0.1)

        # Stop managers
        for manager in self.manager_workers: 
            if manager.processingtype == "process":
                manager.stop(False)
            else:
                manager.stop(fast)
            manager.join()

        print("All workers and managers terminated.")
        sys.exit(0)

