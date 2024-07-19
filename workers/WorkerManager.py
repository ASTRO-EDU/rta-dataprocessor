# Copyright (C) 2024 INAF
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
        self.logger = self.supervisor.logger
        self.name = name
        self.fullname = self.supervisor.name + "-" + name
        self.globalname = "WorkerManager-"+self.fullname
        self.continueall = True
        self.processingtype = self.supervisor.processingtype
        #number max of workers
        self.max_workes = 100
        self.result_socket_type = self.supervisor.manager_result_sockets_type[manager_id]
        self.result_lp_socket = self.supervisor.manager_result_lp_sockets[manager_id]
        self.result_hp_socket = self.supervisor.manager_result_hp_sockets[manager_id]
        self.result_dataflow_type = self.supervisor.manager_result_dataflow_type[manager_id]

        #results
        self.socket_lp_result = self.supervisor.socket_lp_result
        self.socket_hp_result = self.supervisor.socket_hp_result

        self.pid = psutil.Process().pid

        self.context = self.supervisor.context
                
        self.socket_monitoring = self.supervisor.socket_monitoring

        #input queue for thread
        if self.processingtype == "thread":
            self.low_priority_queue = queue.Queue()
            self.high_priority_queue = queue.Queue()
            self.result_lp_queue = queue.Queue()
            self.result_hp_queue = queue.Queue()

        #input queue for processes
        if self.processingtype == "process":
            self.low_priority_queue = multiprocessing.Queue()
            self.high_priority_queue = multiprocessing.Queue()
            self.result_lp_queue = multiprocessing.Queue()
            self.result_hp_queue = multiprocessing.Queue()


        self.monitoringpoint = None
        self.monitoring_thread = None
        self.processing_rates_shared = multiprocessing.Array('f', self.max_workes)
        self.total_processed_data_count_shared = multiprocessing.Array('f', self.max_workes)
        self.worker_status_shared = multiprocessing.Array('f', self.max_workes)

        self.worker_threads = []
        self.worker_processes = []
        self.num_workers = 0

        self.workerstatus=0
        self.workerstatusinit=0

        self.status = "Initialised"

        #process data based on Supervisor state
        self.processdata = 0
        self.processdata_shared = multiprocessing.Value('i', 0)
        self.stopdata = 0

        self._stop_event = threading.Event()  # Used to stop the manager

        #multiprocessing and multithreading locks to write results in order
        if self.processingtype == "thread":
            self.tokenresultslock = threading.Lock()
            self.tokenreadinglock = threading.Lock()
        # if self.processingtype == "process":
        #     self.tokenresultslock_proc = multiprocessing.Lock()
        #     self.tokenreadinglock_proc = multiprocessing.Lock()

        print(f"{self.globalname} started")
        self.logger.system("Started", extra=self.globalname)
        print(f"Socket result parameters: {self.result_socket_type} / {self.result_lp_socket} / {self.result_hp_socket} / {self.result_dataflow_type}")
        self.logger.system(f"Socket result parameters: {self.result_socket_type} / {self.result_lp_socket} / {self.result_hp_socket} / {self.result_dataflow_type}", extra=self.globalname)

    def change_token_results(self):

        if self.processingtype == "thread":

            self.tokenresultslock.acquire()

            for worker in self.worker_threads:
                worker.tokenresult = worker.tokenresult - 1
                if worker.tokenresult < 0:
                    worker.tokenresult = self.num_workers - 1   
                #print(f"W{worker.tokenresult}")
            
            self.tokenresultslock.release() 
            return

        # if self.processingtype == "process":
        #     print("! called W")
        #     with self.tokenresultslock_proc:
        #         for worker in self.worker_processes:
        #             worker.tokenresult = worker.tokenresult - 1
        #             if worker.tokenresult < 0:
        #                 worker.tokenresult = self.num_workers - 1 
        #             print(f"W{worker.tokenresult}")
        #         return

    def change_token_reading(self):
        
        if self.processingtype == "thread":
            self.tokenreadinglock.acquire()
           
            for worker in self.worker_threads:
                worker.tokenreading = worker.tokenreading - 1
                if worker.tokenreading < 0:
                    worker.tokenreading = self.num_workers - 1
                #print(f"R{worker.tokenreading}")

            self.tokenreadinglock.release()
            return

        # if self.processingtype == "process":
        #     print("! to be implemented")
        #     try:
        #         with self.tokenreadinglock_proc:
        #             print("Acquired lock")
        #             for worker in self.worker_processes:
        #                 print(f"Processing worker {worker}")
        #                 worker.tokenreading = worker.tokenreading - 1
        #                 if worker.tokenreading < 0:
        #                     worker.tokenreading = self.num_workers - 1 
        #                 print(f"R{worker.tokenreading}")
        #             print("Released lock")
        #             return
        #     except Exception as e:
        #         print(f"An error occurred: {e}")

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
            self.logger.warning(f"WARNING! It is not possible to create more than {self.max_workes} threads", extra=self.globalname)
        self.num_workers = num_threads

    # to be reimplemented ###
    def start_worker_processes(self, num_processes):
        # Worker processes
        if num_processes > self.max_workes:
            print(f"WARNING! It is not possible to create more than {self.max_workes} process")
            self.logger.warning(f"WARNING! It is not possible to create more than {self.max_workes} process", extra=self.globalname)
        self.num_workers = num_processes


    def run(self):
        self.start_service_threads()

        self.status = "Waiting"

        try:
            while not self._stop_event.is_set():
                time.sleep(1)  # To avoid 100 per cent of CPU comsumption
                #check the status of the workers
                self.workerstatus=0
                self.workerstatusinit=0
                worker_id = 0
                for process in self.worker_processes:
                    status = self.worker_status_shared[worker_id]
                    if status == 0:
                        self.workerstatusinit = self.workerstatusinit + 1
                    else:
                        self.workerstatus = self.workerstatus + status
                    worker_id = worker_id + 1
                for thread in self.worker_threads:
                    if thread.status == 0:
                        self.workerstatusinit = self.workerstatusinit + 1
                    else:
                        self.workerstatus = self.workerstatus + thread.status
                if self.num_workers != self.workerstatusinit:
                    self.workerstatus = self.workerstatus / (self.num_workers-self.workerstatusinit)
                #print(f"Manager workers status {self.globalname} {self.workerstatusinit} {self.workerstatus}")
            print(f"Manager stop {self.globalname}")
            self.logger.system("Manager stop", extra=self.globalname)
        except KeyboardInterrupt:
            print("Keyboard interrupt received. Terminating.")
            self.logger.system("Keyboard interrupt received. Terminating.", extra=self.globalname)
            self.stop_internalthreads()
            self.continueall = False

    def clean_queue(self):
        print("Cleaning queues...")
        self.logger.system("Cleaning queues...", extra=self.globalname)

        if not self.low_priority_queue.empty():
            print(f"   - low_priority_queue size {self.low_priority_queue.qsize()}")
            self.logger.system(f"   - low_priority_queue size {self.low_priority_queue.qsize()}", extra=self.globalname)
            while not self.low_priority_queue.empty():
                item = self.low_priority_queue.get_nowait()
            print(f"   - low_priority_queue empty")
            self.logger.system(f"   - low_priority_queue empty", extra=self.globalname)

        if not self.high_priority_queue.empty():
            print(f"   - high_priority_queue size {self.high_priority_queue.qsize()}")
            self.logger.system(f"   - high_priority_queue size {self.high_priority_queue.qsize()}", extra=self.globalname)
            while not self.high_priority_queue.empty():
                item = self.high_priority_queue.get_nowait()
            print(f"   - high_priority_queue empty")
            self.logger.system(f"   - high_priority_queue empty", extra=self.globalname)

        if not self.result_lp_queue.empty():
            print(f"   - result_lp_queue size {self.result_lp_queue.qsize()}")
            self.logger.system(f"   - result_lp_queue size {self.result_lp_queue.qsize()}", extra=self.globalname)
            while not self.result_lp_queue.empty():
                item = self.result_lp_queue.get_nowait()
            print(f"   - result_lp_queue empty")
            self.logger.system(f"   - result_lp_queue empty", extra=self.globalname)

        if not self.result_hp_queue.empty():
            print(f"   - result_hp_queue size {self.result_hp_queue.qsize()}")
            self.logger.system(f"   - result_hp_queue size {self.result_hp_queue.qsize()}", extra=self.globalname)
            while not self.result_hp_queue.empty():
                item = self.result_hp_queue.get_nowait()
            print(f"   - result_hp_queue empty")
            self.logger.system(f"   - result_hp_queue empty", extra=self.globalname)

        print("End cleaning queues")
        self.logger.system("End cleaning queues", extra=self.globalname)

    def stop(self, fast=False):
        if self.processingtype == "process":
            if fast == False:
                print("Closing queues...")
                self.logger.system("Closing queues...", extra=self.globalname)

                try:
                    print(f"   - low_priority_queue size {self.low_priority_queue.qsize()}")
                    self.logger.system(f"   - low_priority_queue size {self.low_priority_queue.qsize()}", extra=self.globalname)
                    while not self.low_priority_queue.empty():
                        item = self.low_priority_queue.get_nowait()
                    self.low_priority_queue.close()
                    self.low_priority_queue.cancel_join_thread() 
                    print(f"   - low_priority_queue empty")
                    self.logger.system(f"   - low_priority_queue empty", extra=self.globalname)
                except Exception as e:
                    # Handle any other unexpected exceptions
                    print(f"ERROR in worker stop low_priority_queue cleaning: {e}")
                    self.logger.error(f"ERROR in worker stop low_priority_queue cleaning: {e}", extra=self.globalname)

                try:
                    print(f"   - high_priority_queue size {self.high_priority_queue.qsize()}")
                    self.logger.system(f"   - high_priority_queue size {self.high_priority_queue.qsize()}", extra=self.globalname)
                    while not self.high_priority_queue.empty():
                        item = self.high_priority_queue.get_nowait()
                    self.high_priority_queue.close()
                    self.high_priority_queue.cancel_join_thread() 
                    print(f"   - high_priority_queue empty")
                    self.logger.system(f"   - high_priority_queue empty", extra=self.globalname)
                except Exception as e:
                    # Handle any other unexpected exceptions
                    print(f"ERROR in worker stop high_priority_queue cleaning: {e}")
                    self.logger.error(f"ERROR in worker stop high_priority_queue cleaning: {e}", extra=self.globalname)

                try:
                    print(f"   - result_lp_queue size {self.result_lp_queue.qsize()}")
                    self.logger.system(f"   - result_lp_queue size {self.result_lp_queue.qsize()}", extra=self.globalname)
                    while not self.result_lp_queue.empty():
                        item = self.result_lp_queue.get_nowait()
                    self.result_lp_queue.close()
                    self.result_lp_queue.cancel_join_thread() 
                    print(f"   - result_lp_queue empty")
                    self.logger.system(f"   - result_lp_queue empty", extra=self.globalname)
                except Exception as e:
                    # Handle any other unexpected exceptions
                    print(f"ERROR in worker stop result_lp_queue cleaning: {e}")
                    self.logger.error(f"ERROR in worker stop result_lp_queue cleaning: {e}", extra=self.globalname)

                try:
                    print(f"   - result_hp_queue size {self.result_hp_queue.qsize()}")
                    self.logger.system(f"   - result_hp_queue size {self.result_hp_queue.qsize()}", extra=self.globalname)
                    while not self.result_hp_queue.empty():
                        item = self.result_hp_queue.get_nowait()
                    self.result_hp_queue.close()
                    self.result_hp_queue.cancel_join_thread() 
                    print(f"   - result_hp_queue empty")
                    self.logger.system(f"   - result_hp_queue empty", extra=self.globalname)
                except Exception as e:
                    # Handle any other unexpected exceptions
                    print(f"ERROR in worker stop result_hp_queue cleaning: {e}")
                    self.logger.error(f"ERROR in worker stop result_hp_queue cleaning: {e}", extra=self.globalname)

                print("End closing queues")
                self.logger.system("End closing queues", extra=self.globalname)

        for process in self.worker_processes:
            process.stop()
            process.join()
        # Stop worker threads
        for thread in self.worker_threads:
            thread.stop()
            thread.join()       
        self._stop_event.set()  # Set the stop event to exit from this thread
        self.stop_internalthreads()
        self.status = "End"


    def stop_internalthreads(self):
        print("Stopping Manager internal threads...")
        self.logger.system("Stopping Manager internal threads...", extra=self.globalname)
        # Stop monitoring thread
        self.monitoring_thread.stop()
        self.monitoring_thread.join()
        print("All Manager internal threads terminated.")
        self.logger.system("All Manager internal threads terminated.", extra=self.globalname)

    def configworkers(self, configuration):
        if self.processingtype == "process":
            for worker in self.worker_processes:
                worker.config(configuration)   
        
        if self.processingtype == "thread":
            for worker in self.worker_threads:
                worker.config(configuration)        

