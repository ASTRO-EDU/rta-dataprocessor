# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import queue
import multiprocessing
import time
from multiprocessing import Event, Queue, Process
from threading import Timer
import psutil

class WorkerProcess(Process):
    def __init__(self, worker_id, manager, name, worker):
        super().__init__()

        self.worker = worker
        self.manager = manager
        self.supervisor = manager.supervisor
        self.worker_id = worker_id
        self.name = name
        self.pidprocess = psutil.Process().pid

        self.worker.init(self.manager, self.supervisor)

        self.globalname = f"WorkerProcess-{self.supervisor.name}-{self.manager.name}-{self.name}-{self.worker_id}"

        self.low_priority_queue = self.manager.low_priority_queue
        self.high_priority_queue = self.manager.high_priority_queue
        self.result_queue = self.manager.result_queue
        self.monitoringpoint = self.manager.monitoringpoint

        #monitoring
        self.start_time = time.time()
        self.next_time = self.start_time
        self.processed_data_count = 0
        self.total_processed_data_count = 0
        self.processing_rate = 0

        self._stop_event = Event()  # Set the stop event

        #0 initialised
        #1 waiting
        #2 processing
        #3 stop
        self.manager.worker_status_shared[self.worker_id] = 0 #initialised

        self.start_timer(1)
        self.timer.cancel()

        print(f"{self.globalname} started {self.pidprocess}")

    def stop(self):
        self.timer.cancel()
        time.sleep(0.1)
        self._stop_event.set()  # Set the stop event

    def run(self):
        self.start_timer(10)

        try:
            while not self._stop_event.is_set():
                time.sleep(0.0001) #must be 0

                if self.manager.processdata_shared.value == 1:

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
                            self.manager.worker_status_shared[self.worker_id] = 1 #waiting for new data
                            pass  # Continue if both queues are empty
        except Exception:
            pass

        self.timer.cancel()
        self.manager.worker_status_shared[self.worker_id] = 100 #stop
        print(f"WorkerProcess stop {self.globalname}")

    def start_timer(self, interval):
        self.timer = Timer(interval, self.calcdatarate)
        self.timer.start()

    def calcdatarate(self):
        elapsed_time = time.time() - self.next_time
        self.next_time = time.time()
        self.processing_rate = self.processed_data_count / elapsed_time
        self.manager.processing_rates_shared[self.worker_id] = self.processing_rate
        self.total_processed_data_count += self.processed_data_count
        self.manager.total_processed_data_count_shared[self.worker_id] = self.total_processed_data_count
        print(f"{self.globalname} Rate Hz {self.processing_rate:.1f} Current events {self.processed_data_count} Total events {self.total_processed_data_count} Queues {self.manager.low_priority_queue.qsize()} {self.manager.high_priority_queue.qsize()} {self.manager.result_queue.qsize()}")
        self.processed_data_count = 0

        if not self._stop_event.is_set():
            self.start_timer(10)

    def process_data(self, data, priority):
        #print(f"Thread-{self.worker_id} Priority-{priority} processing data. Queues size: {self.low_priority_queue.qsize()} {self.high_priority_queue.qsize()}")
        # Increment the processed data count and calculate the rate
        self.manager.worker_status_shared[self.worker_id] = 2 #processig new data
        self.processed_data_count += 1

        dataresult = self.worker.process_data(data)
