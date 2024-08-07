# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import threading
import time
import psutil

class MonitoringPoint:
    def __init__(self, manager):
        self.manager = manager
        self.processOS = psutil.Process(self.manager.pid)
        self.lock = threading.Lock()
        self.data = {}
        header = {
                "type": 1,
                "time": 0,  # Replace with actual timestamp if needed
                "pidsource": self.manager.fullname,
                "pidtarget": "*"
        }
        self.data["header"] = header
        self.data["workermanagerstatus"] = "Initialised"  # Append the "status" key to the data dictionary
        procinfo = {
            "cpu_percent": 0,
            "memory_usage": 0
        }
        self.data["procinfo"] = procinfo
        #size of two low and high priority queues
        self.data["queue_lp_size"] = 0
        self.data["queue_hp_size"] = 0
        self.processing_rates = {}
        self.processing_tot_events = {}
        self.worker_status = {}
        print("MonitoringPoint initialised")

    def update(self, key, value):
        #with self.lock:
            self.data[key] = value
            #if key in self.data:
            #    self.data[key] = value
            #else:
            #    print(f"Error: Key '{key}' not found in the data dictionary.")

    def get_data(self):
        #with self.lock:
            self.resource_monitor()
            self.data["header"]["time"] = time.time()
            self.set_status(self.manager.status)
            self.data["stopdatainput"] = self.manager.stopdata
            self.update("queue_lp_size", self.manager.low_priority_queue.qsize())
            self.update("queue_hp_size", self.manager.high_priority_queue.qsize())
            self.update("queue_lp_result_size", self.manager.result_lp_queue.qsize())
            self.update("queue_hp_result_size", self.manager.result_hp_queue.qsize())
            self.update("workersstatusinit", self.manager.workersstatusinit)
            self.update("workersstatus", self.manager.workersstatus)
            self.update("workersname", self.manager.workersname)


            if self.manager.processingtype == "process":
                for worker in self.manager.worker_processes:
                    #print(f"Monitor {worker.globalname} {worker.processing_rate}")
                    self.processing_rates[worker.worker_id] = self.manager.processing_rates_shared[worker.worker_id]
                    self.processing_tot_events[worker.worker_id] = self.manager.total_processed_data_count_shared[worker.worker_id]
                    self.worker_status[worker.worker_id] = int(self.manager.worker_status_shared[worker.worker_id])
            if self.manager.processingtype == "thread":
                for worker in self.manager.worker_threads:
                    self.processing_rates[worker.worker_id] = worker.processing_rate
                    self.processing_tot_events[worker.worker_id] = worker.total_processed_data_count
                    self.worker_status[worker.worker_id] = int(worker.status)
            self.data["worker_rates"] = self.processing_rates
            self.data["worker_tot_events"] = self.processing_tot_events
            self.data["worker_status"] = self.worker_status
            return self.data
 
    def set_status(self, new_status):
        #with self.lock:
            self.data["workermanagerstatus"] = new_status

    def get_status(self):
        #with self.lock:
            return self.data["workermanagerstatus"]

    def resource_monitor(self):
        
        # Monitoraggio CPU
        self.data["procinfo"]["cpu_percent"] = self.processOS.cpu_percent(interval=1)

        # Monitoraggio occupazione di memoria
        self.data["procinfo"]["memory_usage"] = self.processOS.memory_info()

        #print(f"CPU: {cpu_percent}% | Memroy: {memory_usage}% | I/O Disco: {disk_io} | I/O Rete: {network_io}")


