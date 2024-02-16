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
import queue
import threading
import time
import psutil
import multiprocessing

class WorkerManager(threading.Thread):
    #manager_type="Process" or manager_type="Thread"
    def __init__(self, manager_id, supervisor, name = "None"):
        super().__init__()
        #unique ID within the supervisor
        self.manager_id = manager_id
        self.supervisor = supervisor
        self.config = self.supervisor.config
        self.name = name
        self.globalname = "WorkerManager-"+self.supervisor.name + "-" + name
        self.continueall = True
        self.processingtype = self.supervisor.processingtype
        #number max of workers
        self.max_workes = 100
        self.result_socket_type = self.supervisor.manager_result_sockets_type[manager_id]
        self.result_socket = self.supervisor.manager_result_sockets[manager_id]
        self.result_dataflow_type = self.supervisor.manager_result_dataflow_type[manager_id]

        #results
        self.socket_result = self.supervisor.socket_result

        self.pid = psutil.Process().pid

        self.context = self.supervisor.context
                
        self.socket_monitoring = self.supervisor.socket_monitoring

        #input queue for thread
        if self.processingtype == "thread":
            self.low_priority_queue = queue.Queue()
            self.high_priority_queue = queue.Queue()
            self.result_queue = queue.Queue()

        #input queue for processes
        if self.processingtype == "process":
            self.low_priority_queue = multiprocessing.Queue()
            self.high_priority_queue = multiprocessing.Queue()
            self.result_queue = multiprocessing.Queue()


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
        self.stopdata = 0
        self.stopdata_shared = multiprocessing.Value('i', 0)

        self._stop_event = threading.Event()  # Used to stop the manager

        print(f"{self.globalname} started")
        print(f"Socket result parameters: {self.result_socket_type} / {self.result_socket} / {self.result_dataflow_type}")

    def set_processdata(self, processdata):
        self.processdata = processdata

        if self.processdata == 1:
            self.status = "Processing"
        if self.processdata == 0:
            self.status = "Waiting"

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

    #to be reimplemented ###
    def start_worker_threads(self, num_threads):
        #Worker threads
        if num_threads > self.max_workes:
            print(f"WARNING! It is not possible to create more than {self.max_workes} threads")
        self.num_workers = num_threads
        # for i in range(num_threads):
        #     thread = WorkerThread(i, self, "name")
        #     self.worker_threads.append(thread)
        #     thread.start()

    # to be reimplemented ###
    def start_worker_processes(self, num_processes):
        # Worker processes
        if num_processes > self.max_workes:
            print(f"WARNING! It is not possible to create more than {self.max_workes} threads")
        self.num_workers = num_processes
        # for i in range(num_processes):
        #     process = WorkerProcess(i, self, self.processdata_shared, "name")
        #     self.worker_processes.append(process)
        #     process.start()


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

    def clean_queue(self):
        print("Cleaning queues...")

        print(f"   - low_priority_queue size {self.low_priority_queue.qsize()}")
        while not self.low_priority_queue.empty():
            item = self.low_priority_queue.get_nowait()
        print(f"   - low_priority_queue empty")
 
        print(f"   - high_priority_queue size {self.high_priority_queue.qsize()}")
        while not self.high_priority_queue.empty():
            item = self.high_priority_queue.get_nowait()
        print(f"   - high_priority_queue empty")

        print(f"   - result_queue size {self.result_queue.qsize()}")
        while not self.result_queue.empty():
            item = self.result_queue.get_nowait()
        print(f"   - result_queue empty")

        print("End cleaning queues")

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

                print(f"   - result_queue size {self.result_queue.qsize()}")
                while not self.result_queue.empty():
                    item = self.result_queue.get_nowait()
                self.result_queue.close()
                self.result_queue.cancel_join_thread() 
                print(f"   - result_queue empty")

                print("End closing queues")

        for process in self.worker_processes:
            process.stop()
            process.join()
        # Stop worker threads
        for thread in self.worker_threads:
            thread.stop()
            thread.join()       
        self._stop_event.set()  # Set the stop event to exit from this thread
        self.status = "End"


    def stop_processes(self):
        print("Stopping Manager threads...")
        # Stop monitoring thread
        self.monitoring_thread.stop()
        self.monitoring_thread.join()
        print("All Manager threads terminated.")


