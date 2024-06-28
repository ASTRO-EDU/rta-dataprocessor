# Copyright (C) 2024 INAF
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Nicolo' Parmiggiani <nicolo.parmiggiani@inaf.it>
#
from Supervisor import Supervisor
from WorkerManager import WorkerManager
from ConfigurationManager import ConfigurationManager, get_kafka_config
import zmq
import json
import queue
import threading
import signal
import time
import sys
import psutil
import os
from WorkerLogger import WorkerLogger
from confluent_kafka import Consumer, KafkaException, KafkaError
import traceback

class SupervisorKafka(Supervisor):
    def __init__(self, config_file="config.json", name = "None"):
        super().__init__(config_file,name)  
        try:
            if self.datasockettype == "kafka":
                #low priority data stream connection
                kafka_config = get_kafka_config(self.config.get("data_lp_socket"))
                consumer_config = {
                    'bootstrap.servers': kafka_config[0],  # Indirizzo del broker Kafka
                    'group.id':'ooqs_lp',
                    'auto.offset.reset': 'latest'         
                }
                self.consumer_lp = Consumer(consumer_config)
                topic = kafka_config[1]
                self.consumer_lp.subscribe([topic])

                #high priority data stream connection
                kafka_config = get_kafka_config(self.config.get("data_hp_socket"))
                consumer_config = {
                    'bootstrap.servers': kafka_config[0],  # Indirizzo del broker Kafka
                    'group.id':'ooqs_lp',
                    'auto.offset.reset': 'latest'         
                }
                self.consumer_hp = Consumer(consumer_config)
                topic = kafka_config[1]
                self.consumer_hp.subscribe([topic])
            
            else:
                raise ValueError("Config file: datasockettype must be kafka")
                
    
        except Exception as e:
            # Handle any other unexpected exceptions
            traceback_str = traceback.format_exc()
            print(f"ERROR: An unexpected error occurred: {e} {traceback_str}")
            self.logger.warning(f"ERROR: An unexpected error occurred: {e} {traceback_str}", extra=self.globalname)
            sys.exit(1)


    def load_configuration(self, config_file, name):
        self.config_manager = ConfigurationManager(config_file)
        self.config=self.config_manager.get_configuration(name)
        print(self.config)
        self.manager_result_sockets_type, self.manager_result_dataflow_type, self.manager_result_lp_sockets, self.manager_result_hp_sockets, self.manager_num_workers = self.config_manager.get_workers_config(name)


    def listen_for_lp_data(self):
        try:
            while self.continueall:

                data = self.consumer_lp.poll()  
                
                if data is None:
                    continue

                if data.error():
                    if data.error().code() == KafkaError._PARTITION_EOF:
                        print(f'End of partition reached {data.topic()}/{data.partition()}')
                    elif data.error():
                        raise KafkaException(data.error())
                else:
                    decodeddata = self.decode_data(data)
                    for manager in self.manager_workers: 
                        manager.low_priority_queue.put(decodeddata.value()) 

        except Exception as e:
            # Handle any other unexpected exceptions
            print(f"ERROR: An unexpected error occurred: {e}")
            self.logger.warning(f"ERROR: An unexpected error occurred: {e}", extra=self.globalname)
            sys.exit(1)
        finally:
            # Cleanup
            self.consumer_lp.close()

        print("End listen_for_lp_data")
        self.logger.system("End listen_for_lp_data", extra=self.globalname)

    def listen_for_hp_data(self):
        try:
            while self.continueall:

                data = self.consumer_hp.poll()  
                
                if data is None:
                    continue

                if data.error():
                    if data.error().code() == KafkaError._PARTITION_EOF:
                        print(f'End of partition reached {data.topic()}/{data.partition()}')
                    elif data.error():
                        raise KafkaException(data.error())
                else:
                    decodeddata = self.decode_data(data)
                    for manager in self.manager_workers: 
                        manager.high_priority_queue.put(decodeddata.value()) 

        except Exception as e:
            # Handle any other unexpected exceptions
            print(f"ERROR: An unexpected error occurred: {e}")
            self.logger.warning(f"ERROR: An unexpected error occurred: {e}", extra=self.globalname)
            sys.exit(1)
        finally:
            # Cleanup
            self.consumer_hp.close()

        print("End listen_for_lp_data")
        self.logger.system("End listen_for_hp_data", extra=self.globalname)

    