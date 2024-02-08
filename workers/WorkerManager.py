# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
from MonitoringPoint import MonitoringPoint
from WorkerThread import WorkerThread
from WorkerProcess import WorkerProcess
from MonitoringThread import MonitoringThread
import json
import zmq
import queue
import threading
import time
import sys
import psutil
import threading
import multiprocessing

class WorkerManager(threading.Thread):
    #manager_type="Process" or manager_type="Thread"
    def __init__(self, supervisor, name = "None"):
        super().__init__()
        self.supervisor = supervisor
        self.config_data = self.supervisor.config_data
        self.name = name
        self.globalname = "WorkerManager-"+self.supervisor.name + "-" + name
        self.continueall = True
        self.processingtype = self.supervisor.processingtype
        #number max of workers
        self.max_workes = 100

        self.pid = psutil.Process().pid

        self.context = self.supervisor.context
                
        self.socket_monitoring = self.supervisor.socket_monitoring

        self.socket_result = self.context.socket(zmq.PUSH)
        self.socket_result.connect(self.config_data["result_socket_push"])

        #thread
        if self.processingtype == "thread":
            self.low_priority_queue = queue.Queue()
            self.high_priority_queue = queue.Queue()

        #processes
        if self.processingtype == "process":
            self.low_priority_queue = multiprocessing.Queue()
            self.high_priority_queue = multiprocessing.Queue()

        self.monitoringpoint = None
        self.monitoring_thread = None
        self.processing_rates_shared = multiprocessing.Array('f', self.max_workes)
        self.total_processed_data_count_shared = multiprocessing.Array('f', self.max_workes)

        self.worker_threads = []
        self.worker_processes = []
        self.num_workers = 0

        self.status = "Initialised"

        #process data based on Supervisor state
        self.processdata = 0
        self.processdata_shared = multiprocessing.Value('i', 0)
        self.suspenddata = 0
        self.suspenddata_shared = multiprocessing.Value('i', 0)

        self._stop_event = threading.Event()  # Used to stop the manager

        print(f"{self.globalname} started")

    def set_processdata(self, processdata):
        self.processdata = processdata

        if self.processingtype == "process":
            self.processdata_shared.value = processdata
        
        if self.processingtype == "thread":
            for worker in self.worker_threads:
                worker.set_processdata(self.processdata)

    def start_service_threads(self):
        #Monitoring thread
        self.monitoringpoint = MonitoringPoint(self)
        self.monitoring_thread = MonitoringThread(self.socket_monitoring, self.monitoringpoint)
        self.monitoring_thread.start()

    #to be reimplemented
    def start_worker_threads(self, num_threads=5):
        #Worker threads
        if num_threads > self.max_workes:
            print(f"WARNING! It is not possible to create more than {self.max_workes} threads")
        self.num_workers = num_threads
        for i in range(num_threads):
            thread = WorkerThread(i, self)
            self.worker_threads.append(thread)
            thread.start()

    # to be reimplemented
    def start_worker_processes(self, num_processes=5):
        # Worker processes
        if num_processes > self.max_workes:
            print(f"WARNING! It is not possible to create more than {self.max_workes} threads")
        self.num_workers = num_processes
        for i in range(num_processes):
            process = WorkerProcess(i, self, self.processdata_shared)
            self.worker_processes.append(process)
            process.start()


    def run(self):
        self.start_service_threads()

        self.status = "Waiting"

        try:
            while not self._stop_event.is_set():
                time.sleep(1)  # To avoid 100 per cent of CPU comsumption
            print(f"Manager stop {self.globalname}")
        except KeyboardInterrupt:
            print("Keyboard interrupt received. Terminating.")
            self.stop_processes()
            self.continueall = False

    def stop(self, fast=False):
        self.stop_processes()
        if self.processingtype == "process":
            if fast == False:
                print("Closing queues...")

                print(f"   - low_priority_queue size {self.low_priority_queue.qsize()}")
                while not self.low_priority_queue.empty():
                    item = self.low_priority_queue.get_nowait()
                self.low_priority_queue.close()
                self.low_priority_queue.cancel_join_thread() 
                print(f"   - low_priority_queue empty")

                print(f"   - high_priority_queue size {self.high_priority_queue.qsize()}")
                while not self.high_priority_queue.empty():
                    item = self.high_priority_queue.get_nowait()
                self.high_priority_queue.close()
                self.high_priority_queue.cancel_join_thread() 
                print(f"   - high_priority_queue empty")

                print("End closing queues")
        self._stop_event.set()  # Set the stop event to exit from this thread

    def stop_processes(self):
        print("Stopping Manager threads...")
        # Stop monitoring thread
        self.monitoring_thread.stop()
        self.monitoring_thread.join()
        print("All Manager threads terminated.")


