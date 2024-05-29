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
import multiprocessing
import zmq
import json
import queue
import threading
import signal
import time
import sys
import psutil
import os

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
            self.socket_lp_result = [None] * 100
            self.socket_hp_result = [None] * 100

        except Exception as e:
           # Handle any other unexpected exceptions
           print(f"ERROR: An unexpected error occurred: {e}")
           sys.exit(1)

        else:

            self.manager_workers = []

            #process data based on Supervisor state
            self.processdata = 0
            self.stopdata = True

            # Set up signal handlers
            try:
                signal.signal(signal.SIGTERM, self.handle_signals)
                signal.signal(signal.SIGINT, self.handle_signals)
            except ValueError:
                print("WARNING! Signal only works in main thread. It is not possible to set up signal handlers!")
            self.status = "Initialised"

            print(f"{self.globalname} started")

    def load_configuration(self, config_file, name):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        self.manager_result_sockets_type, self.manager_result_dataflow_type, self.manager_result_lp_sockets, self.manager_result_hp_sockets, self.manager_num_workers = self.config_manager.get_workers_config(name)

    def start_service_threads(self):

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

        self.command_thread = threading.Thread(target=self.listen_for_result, daemon=True)
        self.command_thread.start()   

    def setup_result_channel(self, manager, indexmanager):
        #output sockert
        self.socket_lp_result[indexmanager] = None
        self.socket_hp_result[indexmanager] = None
        self.context = zmq.Context()

        if manager.result_lp_socket != "none":
            if manager.result_socket_type == "pushpull":
                self.socket_lp_result[indexmanager] = self.context.socket(zmq.PUSH)
                self.socket_lp_result[indexmanager].connect(manager.result_lp_socket)
                print(f"---result lp socket pushpull {manager.globalname} {manager.result_lp_socket}")

            if manager.result_socket_type == "pubsub":
                self.socket_lp_result[indexmanager] = self.context.socket(zmq.PUB)
                self.socket_lp_result[indexmanager].bind(manager.result_lp_socket)
                print(f"---result lp socket pushpull {manager.globalname} {manager.result_lp_socket}")

        if manager.result_hp_socket != "none":
            if manager.result_socket_type == "pushpull":
                self.socket_hp_result[indexmanager] = self.context.socket(zmq.PUSH)
                self.socket_hp_result[indexmanager].connect(manager.result_hp_socket)
                print(f"---result hp socket pushpull {manager.globalname} {manager.result_hp_socket}")

            if manager.result_socket_type == "pubsub":
                self.socket_hp_result[indexmanager] = self.context.socket(zmq.PUB)
                self.socket_hp_result[indexmanager].bind(manager.result_hp_socket)
                print(f"---result hp socket pushpull {manager.globalname} {manager.result_hp_socket}")


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
            #time.sleep(0.0001)
            indexmanager = 0
            for manager in self.manager_workers:
                self.send_result(manager, indexmanager) 
                indexmanager = indexmanager + 1
        print("End listen_for_result")

    def send_result(self, manager, indexmanager):
        if manager.result_lp_queue.qsize() == 0 and manager.result_hp_queue.qsize() == 0:
            return

        data = None
        channel = -1
        try:
            channel = 1
            data = manager.result_hp_queue.get_nowait()
        except Exception as e:
            try:
                channel = 0
                data = manager.result_lp_queue.get_nowait()
            except queue.Empty:
                return
            except Exception as e:
                print(f"WARNING: {e}")
                return

        if channel == 0:
            if manager.result_lp_socket == "none":
                #print("WARNING: no lp socket result available to send results")
                return
            if manager.result_dataflow_type == "string" or manager.result_dataflow_type == "filename":
                try:
                    data = str(data)
                    self.socket_lp_result[indexmanager].send_string(data)
                except Exception as e:
                    print(f"ERROR: data not in string format to be send to : {e}")
            if manager.result_dataflow_type == "binary":
                try:
                    #data = str(data)
                    self.socket_lp_result[indexmanager].send(data)
                except Exception as e:
                    print(f"ERROR: data not in binary format to be send to socket_result: {e}")

        if channel == 1:
            if manager.result_hp_socket == "none":
                #print("WARNING: no socket hp result available to send results")
                return
            if manager.result_dataflow_type == "string" or manager.result_dataflow_type == "filename":
                try:
                    data = str(data)
                    self.socket_hp_result[indexmanager].send_string(data)
                except Exception as e:
                    print(f"ERROR: data not in string format to be send to : {e}")
            if manager.result_dataflow_type == "binary":
                try:
                    #data = str(data)
                    self.socket_hp_result[indexmanager].send(data)
                except Exception as e:
                    print(f"ERROR: data not in binary format to be send to socket_result: {e}")

    def listen_for_lp_data(self):
        while self.continueall:
            if not self.stopdata:
                data = self.socket_lp_data.recv()
                for manager in self.manager_workers:
                    decodeddata = self.decode_data(data)  
                    manager.low_priority_queue.put(decodeddata) 
        print("End listen_for_lp_data")

    def listen_for_hp_data(self):
        while self.continueall:
            if not self.stopdata:
                data = self.socket_hp_data.recv()
                for manager in self.manager_workers: 
                    decodeddata = self.decode_data(data)
                    manager.high_priority_queue.put(decodeddata) 
        print("End listen_for_hp_data")

    def listen_for_lp_string(self):
        while self.continueall:
            if not self.stopdata:
                data = self.socket_lp_data.recv_string()
                for manager in self.manager_workers: 
                    manager.low_priority_queue.put(data) 
        print("End listen_for_lp_string")

    def listen_for_hp_string(self):
        while self.continueall:
            if not self.stopdata:
                data = self.socket_hp_data.recv_string()
                for manager in self.manager_workers: 
                    manager.high_priority_queue.put(data) 
        print("End listen_for_hp_string")

    #to be reimplemented ####
    #Open the file before load it into the queue. For "dataflowtype": "file"
    #Return an array of data and the size of the array
    def open_file(self, filename):
        f = [filename]
        return f, 1

    def listen_for_lp_file(self):
        while self.continueall:
            if not self.stopdata:
                filename = self.socket_lp_data.recv()
                for manager in self.manager_workers: 
                    data, size = self.open_file(filename) 
                    for i in range(size):
                        manager.low_priority_queue.put(data[i]) 
        print("End listen_for_lp_file")

    def listen_for_hp_file(self):
        while self.continueall:
            if not self.stopdata:
                filename = self.socket_hp_data.recv()
                for manager in self.manager_workers:
                    data, size = self.open_file(filename) 
                    for i in range(size):
                        manager.high_priority_queue.put(data[i])
        print("End listen_for_hp_file")

    def listen_for_commands(self):
        while self.continueall:
            print("Waiting for commands...")
            #try:
            command = json.loads(self.socket_command.recv_string())
            self.process_command(command)
            #except zmq.error.ZMQError:
            #    print("WARNING! zmq.error.ZMQError")
 

        print("End listen_for_commands")

    def command_shutdown(self):
        self.status = "Shutdown"
        self.stop_all(False)
    
    def command_cleanedshutdown(self):
        if self.status == "Processing":
            self.status = "EndingProcessing"
            self.command_stopdata()
            for manager in self.manager_workers:
                print(f"Trying to stop {manager.globalname}...")
                manager.status = "EndingProcessing"
                while manager.low_priority_queue.qsize() != 0 or manager.high_priority_queue.qsize() != 0:
                    print(f"Queues data of manager {manager.globalname} have size {manager.low_priority_queue.qsize()} {manager.high_priority_queue.qsize()}")
                    time.sleep(0.2)            
                while manager.result_lp_queue.qsize() != 0 or manager.result_hp_queue.qsize() != 0:
                    print(f"Queues result of manager {manager.globalname} have size {manager.result_lp_queue.qsize()} {manager.result_hp_queue.qsize()}")
                    time.sleep(0.2) 
                manager.status = "Shutdown"
        else:
            print("WARNING! Not in Processing state for a cleaned shutdown. Force the shutdown.") 
        
        self.status = "Shutdown"
        self.stop_all(False)


    def command_reset(self):
        if self.status == "Processing" or self.status == "Waiting":
            self.command_stop()
            for manager in self.manager_workers:
                print(f"Trying to reset {manager.globalname}...")
                manager.clean_queue()
                print(f"Queues of manager {manager.globalname} have size {manager.low_priority_queue.qsize()} {manager.high_priority_queue.qsize()} {manager.result_lp_queue.qsize()} {manager.result_hp_queue.qsize()}")
            self.status = "Waiting"

    def command_start(self):
        self.command_startprocessing()
        self.command_startdata()

    def command_stop(self):
        self.command_stopdata()
        self.command_stopprocessing()

    def command_startprocessing(self):
        self.status = "Processing"
        for manager in self.manager_workers:
            manager.status = "Processing"
            manager.set_processdata(1)

    def command_stopprocessing(self):
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
        
        subtype_value = command['header']['subtype']
        pidtarget = command['header']['pidtarget']
        pidsource = command['header']['pidsource']
        if pidtarget == self.name or pidtarget == "all".lower() or pidtarget == "*":
            print(f"Received command: {command}")
            if subtype_value == "shutdown":
                self.command_shutdown()  
            if subtype_value == "cleanedshutdown":
                self.command_cleanedshutdown()
            if subtype_value == "getstatus":
                for manager in self.manager_workers:
                    manager.monitoring_thread.sendto(pidsource)
            if subtype_value == "start": #data processing + data
                    self.command_start()
            if subtype_value == "stop": #data processing + data
                    self.command_stop()
            if subtype_value == "startprocessing": #data processing
                    self.command_startprocessing()
            if subtype_value == "stopprocessing": #data processing
                    self.command_stopprocessing()
            if subtype_value == "reset": #reset the data processor
                    self.command_reset()
            if subtype_value == "stopdata": #data acquisition
                    self.command_stopdata()
            if subtype_value == "startdata": #data acquisition
                    self.command_startdata()


    def stop_all(self, fast=False):
        print("Stopping all workers and managers...")

        #self.command_stopdata()
        self.command_stop()
        time.sleep(0.1)

        # Stop managers
        for manager in self.manager_workers: 
            if manager.processingtype == "process":
                manager.stop(fast)
            else:
                manager.stop(fast)
            manager.join()

        # Stop all internal thread
        self.continueall = False
        
        #self.socket_lp_data.close()
        #self.socket_hp_data.close()
        #self.socket_monitoring.close()
        #self.socket_command.close()
        #self.socket_result[0].close()

        #self.lp_data_thread.join()
        #print("self.lp_data_thread.join()")
        #self.hp_data_thread.join()
        #print("self.hp_data_thread.join()")
        #self.result_thread.join()
        #print("self.result_thread.join()")
        
        # Clear any remaining items in the queue
        #for manager in self.manager_workers:
        #    manager.clean_queue()

        print("All Supervisor workers and managers and internal threads terminated.")
        #sys.exit(0)

