# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#    Nicolo' Parmiggiani <nicolo.parmiggiani@inaf.it>

import zmq
import sys
import threading
from ConfigurationManager import ConfigurationManager

class Forwarder:
    def __init__(self, conf_file_path, processname):
        
        
        self.processname = processname
        self.load_configuration(config_file_path, processname)

        
        
        # Creazione del contesto ZMQ
        self.context = zmq.Context()

    
    def start(self):
        #Creazione del socket SUB (per ricevere messaggi dai publisher)
        self.frontend = self.context.socket(zmq.SUB)
        self.frontend.bind(self.config.get("frontend_socket"))
        self.frontend.setsockopt_string(zmq.SUBSCRIBE, "")  # Sottoscrive a tutti i messaggi

        # Creazione del socket PUB (per inoltrare i messaggi ai subscriber)
        self.backend = self.context.socket(zmq.PUB)
        self.backend.bind(self.config.get("backend_socket"))

        # Utilizzo di zmq.proxy per collegare i socket
        try:
            zmq.proxy(self.frontend, self.backend)
        except zmq.ContextTerminated:
            print("Context terminated")
        finally:
            # Pulizia delle risorse
            self.frontend.close()
            self.backend.close()
            self.context.term()
        

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        
        
if __name__ == "__main__":
    
    config_file_path = sys.argv[1]
    forwarder = Forwarder(config_file_path,"MonitoringForward")
    forwarder.start()