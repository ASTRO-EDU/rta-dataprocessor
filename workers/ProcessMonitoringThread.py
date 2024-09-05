# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import zmq
import json
import sys
from ConfigurationManager import ConfigurationManager
import threading
import time
import queue

class MonitoringConsumerThread:
    def __init__(self, config_file_path, processname=[]):
        
        self.processname = processname
        self.load_configuration(config_file_path, processname)
        self.context = zmq.Context()
        self.run_monitoring = True
    
        print(self.config.get("monitoring_socket"))
        if(self.config.get("monitoring_forwarder")=="on"):
            self.socket_monitoring = self.context.socket(zmq.SUB)
            self.socket_monitoring.connect(self.config.get("monitoring_socket"))
            self.socket_monitoring.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
        else:
            
            self.socket_monitoring = self.context.socket(zmq.PULL)
            self.socket_monitoring.bind(self.config.get("monitoring_socket"))
        
        self.socket_monitoring.setsockopt(zmq.RCVTIMEO, 1000)
          
        self.monitoring_thread = threading.Thread(target=self.receive_and_decode_messages, daemon=True)
        
        self.message_queue  = queue.Queue()
        
    def receive_and_decode_messages(self):

        self.run_monitoring = True
        
        while self.run_monitoring:
            try:
                # Attendi un messaggio con timeout
                message = self.socket_monitoring.recv_string()
                decoded_message = json.loads(message)
                self.message_queue.put(decoded_message)
            except zmq.Again:
                # Questo blocco viene eseguito se scade il timeout
                time.sleep(0.01)
                print("stop monitoring")
                
        
        
    def start_monitoring_thread(self):
        
        self.run_monitoring = True
        self.monitoring_thread.start()
        
        
    def stop_monitoring_thread(self):
        
        self.run_monitoring = False
        self.monitoring_thread.join()

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python ProcessMonitoring.py <config_file>")
        sys.exit(1)

    config_file_path = sys.argv[1]

    # Use the configuration to initialize the MonitoringConsumer
    monitoring_consumer = MonitoringConsumerThread(config_file_path, "Monitoring")

    try:
        monitoring_consumer.receive_and_decode_messages()
    except KeyboardInterrupt:
        print("Consumer stopped.")
