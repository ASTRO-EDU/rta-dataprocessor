import threading
# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import time
import json

class MonitoringThread(threading.Thread):
    def __init__(self, socket_monitoring, monitoringpoint):
        super().__init__(name="Monitoring-Thread")
        self.socket_monitoring = socket_monitoring
        self.monitoringpoint = monitoringpoint
        self._stop_event = threading.Event()  # Usato per segnalare l'arresto
        print("Monitoring-Thread started")

    def stop(self):
        self._stop_event.set()  # Imposta l'evento di arresto

    def run(self):
        while not self._stop_event.is_set():
            monitoring_data =  self.monitoringpoint.get_data()
            monitoring_data_str = json.dumps(monitoring_data)
            #print(monitoring_data_str)
            self.socket_monitoring.send_string(monitoring_data_str)
            time.sleep(1)
    
    def sendto(self, processtargetname):
        monitoring_data =  self.monitoringpoint.get_data()
        monitoring_data['header']['pidtarget'] = processtargetname
        monitoring_data_str = json.dumps(monitoring_data)
        self.socket_monitoring.send_string(monitoring_data_str)
        print("send monitoring")
        print(monitoring_data)


