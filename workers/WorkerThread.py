# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import threading
import queue
import json
import time
from threading import Timer

class WorkerThread(threading.Thread):
    def __init__(self, worker_id, manager, name):
        super().__init__()
        self.manager = manager
        self.supervisor = manager.supervisor
        self.worker_id = worker_id
        self.name = name
        self.globalname = f"WorkerThread-{self.supervisor.name}-{self.manager.name}-{self.name}-{self.worker_id}"

        self.low_priority_queue = self.manager.low_priority_queue
        self.high_priority_queue = self.manager.high_priority_queue
        self.monitoringpoint = self.manager.monitoringpoint

        #monitoring
        self.start_time = time.time()
        self.next_time = self.start_time
        self.processed_data_count = 0
        self.total_processed_data_count = 0
        self.processing_rate = 0

        self._stop_event = threading.Event()  # Set the stop event

        self.processdata = 0

        #0 initialised
        #1 waiting
        #2 processing
        #3 stop
        self.status = 0 #initialised

        print(f"{self.globalname} started")

    def stop(self):
        self.status = 3 #stop
        self._stop_event.set()  # Set the stop event

    def set_processdata(self, processdata1):
        self.processdata=processdata1

    def run(self):
        self.start_timer(10)

        while not self._stop_event.is_set():
            time.sleep(0.00001) #must be 0

            if self.processdata == 1:
                try:
                    # Check and process high-priority queue first
                    high_priority_data = self.high_priority_queue.get_nowait()
                    self.process_data(high_priority_data, priority="High")
                except queue.Empty:
                    try:
                        # Process low-priority queue if high-priority queue is empty
                        low_priority_data = self.low_priority_queue.get(timeout=1)
                        self.process_data(low_priority_data, priority="Low")
                    except queue.Empty:
                        self.status = 1 #waiting for new data
                        pass  # Continue if both queues are empty
        
        self.timer.cancel()
        print(f"WorkerThread stop {self.globalname}")

    def start_timer(self, interval):
        self.timer = Timer(interval, self.calcdatarate)
        self.timer.start()

    def calcdatarate(self):    
        elapsed_time = time.time() - self.next_time
        self.next_time = time.time()
        self.processing_rate = self.processed_data_count / elapsed_time
        self.total_processed_data_count += self.processed_data_count
        print(f"{self.globalname} rate Hz {self.processing_rate:.1f} total events {self.total_processed_data_count}")
        self.processed_data_count = 0

        if not self._stop_event.is_set():
            self.start_timer(10)

    #to be reimplemented ###
    def process_data(self, data, priority):
        #print(f"Thread-{self.worker_id} Priority-{priority} processing data. Queues size: {self.low_priority_queue.qsize()} {self.high_priority_queue.qsize()}")
        # Increment the processed data count and calculate the rate
        self.status = 2 #processig new data
        self.processed_data_count += 1

        #Derive a class and put the code of analysis in this method
