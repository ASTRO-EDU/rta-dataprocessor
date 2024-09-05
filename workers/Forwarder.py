# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Nicolo' Parmiggiani <nicolo.parmiggiani@inaf.it>
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#

import zmq
import sys
import threading
from ConfigurationManager import ConfigurationManager

class Forwarder:
    def __init__(self, conf_file_path, processname):
        
        
        self.processname = processname
        self.load_configuration(config_file_path, processname)

        
        self.context = zmq.Context()
    
    def start(self):
      
        self.frontend = self.context.socket(zmq.PULL)
        self.frontend.bind(self.config.get("forwarder_frontend_socket"))
        
        
        self.backend = self.context.socket(zmq.PUB)
        self.backend.bind(self.config.get("forwarder_backend_socket"))

        
        try:
            zmq.proxy(self.frontend, self.backend)
        except zmq.ContextTerminated:
            print("Context terminated")
        finally:
           
            self.frontend.close()
            self.backend.close()
            self.context.term()
        

    def load_configuration(self, config_file, name="CommandCenter"):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        
        
if __name__ == "__main__":
    
    config_file_path = sys.argv[1]
    forwarder = Forwarder(config_file_path,"Monitoring")
    forwarder.start()