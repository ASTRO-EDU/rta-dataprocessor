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

class MonitoringConsumer:
    def __init__(self, config_file_path, processname=[]):
        
        self.processname = processname
        self.load_configuration(config_file_path, processname)
        self.context = zmq.Context()
        #self.socket_monitoring = self.context.socket(zmq.PULL)
        #self.socket_monitoring.bind(self.config.get("monitoring_socket"))
    
        
        if(self.config.get("monitoring_forwarder")=="on"):
            self.socket_monitoring = self.context.socket(zmq.SUB)
            self.socket_monitoring.connect(self.config.get("backend_socket"))
            self.socket_monitoring.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
        else:
        
            with open(config_file_path, 'r') as file:
                data = json.load(file)
            
            result = [(item['processname'], item['monitoring_socket']) 
            for item in data 
            if item['processname'] not in ["MonitoringForward","CommandCenter"]]
            
            self.context = zmq.Context()
            self.socket_monitoring = self.context.socket(zmq.SUB)
            self.socket_monitoring.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
            
            
            # Stampare il risultato
            for processname, monitoring_socket in result:
                print(f"Processname: {processname}, Monitoring Socket: {monitoring_socket}")
                
                self.socket_monitoring.connect(monitoring_socket)
                #self.socket_monitoring.setsockopt_string(zmq.SUBSCRIBE, "")  # Subscribe to all topics
            

    def receive_and_decode_messages(self):
        
        while True:
            message = self.socket_monitoring.recv_string()
            decoded_message = json.loads(message)
            print("Received and decoded message:", decoded_message)
            

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
    monitoring_consumer = MonitoringConsumer(config_file_path, "MonitoringForward")

    try:
        monitoring_consumer.receive_and_decode_messages()
    except KeyboardInterrupt:
        print("Consumer stopped.")
