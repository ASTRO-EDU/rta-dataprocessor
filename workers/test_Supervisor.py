from MonitoringPoint import MonitoringPoint
from WorkerThread import WorkerThread
from MonitoringThread import MonitoringThread
from WorkerManager import WorkerManager
from WorkerProcess import WorkerProcess
from ConfigurationManager import ConfigurationManager, get_pull_config
from Supervisor import Supervisor
from io import StringIO
from unittest.mock import MagicMock, Mock
import os
import zmq
import json
import queue
import threading
import signal
import time
import sys
import psutil
import pytest
import random 
import keyboard
from busypie import wait_at_most

file_path = '/home/nunzio/rta-dataprocessor/config.json'
file_path_2 = '/home/nunzio/rta-dataprocessor/config_2.json'
context = zmq.Context()
# publisher = context.socket(zmq.PUSH)
# stop_thread = threading.Event()

@pytest.fixture
# def cleanup_after_test(request):
#     def cleanup():
#         test_sup.stop_zmq()
#         print("\nEseguito cleanup dopo il test")
        
#     request.addfinalizer(cleanup)
        

def test_init_pubsub():
    test_sup = Supervisor(file_path,'OOQS1')
    
    assert test_sup.name == 'OOQS1'
    assert test_sup.manager_num_workers == [5,2]
    assert test_sup.manager_result_sockets == ['tcp://localhost:5563','none']
    assert test_sup.manager_result_sockets_type == ['pushpull','none']
    assert test_sup.manager_result_dataflow_type == ['string','none']
    assert test_sup.globalname == "Supervisor-"+'OOQS1'
    assert test_sup.continueall == True
    assert test_sup.pid == psutil.Process().pid
    assert test_sup.processingtype == 'thread'
    assert test_sup.dataflowtype == 'filename'
    assert test_sup.datasockettype == 'pubsub'
    assert test_sup.socket_lp_data.type == zmq.SocketType.SUB
    assert test_sup.socket_hp_data.type == zmq.SocketType.SUB
    assert test_sup.socket_command.type == zmq.SocketType.SUB
    assert test_sup.socket_monitoring.type == zmq.SocketType.PUSH
    assert test_sup.socket_result == [None] * 100
    assert test_sup.manager_workers == []
    assert test_sup.processdata == 0
    assert test_sup.stopdata == False
    assert test_sup.status =='Initialised'
    test_sup.stop_zmq()
    #del(test_sup)

    
def test_init_pushpull():
    test_sup = Supervisor(file_path,'OOQS2')

    assert test_sup.name == 'OOQS2'
    assert test_sup.manager_num_workers == [2]
    assert test_sup.manager_result_sockets == ['none']
    assert test_sup.manager_result_sockets_type == ['none']
    assert test_sup.manager_result_dataflow_type == ['none']
    assert test_sup.globalname == "Supervisor-"+'OOQS2'
    assert test_sup.continueall == True
    assert test_sup.pid == psutil.Process().pid
    assert test_sup.processingtype == 'thread'
    assert test_sup.dataflowtype == 'string'
    assert test_sup.datasockettype == 'pushpull'
    assert test_sup.socket_lp_data.type == zmq.SocketType.PULL
    assert test_sup.socket_hp_data.type == zmq.SocketType.PULL
    assert test_sup.socket_command.type == zmq.SocketType.SUB
    assert test_sup.socket_monitoring.type == zmq.SocketType.PUSH
    assert test_sup.socket_result == [None] * 100
    assert test_sup.manager_workers == []
    assert test_sup.processdata == 0
    assert test_sup.stopdata == False
    assert test_sup.status =='Initialised'
    test_sup.stop_zmq()
    #del(test_sup)


def test_Supervisor_with_different_datasockettype():
    with pytest.raises(ValueError) as raise_test:
        Supervisor(file_path_2,'OOQS3')
    assert str(raise_test.value) == "Config file: datasockettype must be pushpull or pubsub"
    
def test_start_service_threads():
    test_sup = Supervisor(file_path,'OOQS1')
    print("fatto")
    test_sup.start_service_threads()
    
    print("fatto")
    
    if test_sup.dataflowtype == "binary": 
        assert test_sup.lp_data_thread._target == test_sup.listen_for_lp_data
        assert test_sup.lp_data_thread.is_alive()
        assert test_sup.hp_data_thread._target == test_sup.listen_for_hp_data
        assert test_sup.hp_data_thread.is_alive()
    elif test_sup.dataflowtype == "filename":
        assert test_sup.lp_data_thread._target == test_sup.listen_for_lp_file
        assert test_sup.lp_data_thread.is_alive()
        assert test_sup.hp_data_thread._target == test_sup.listen_for_hp_file
        assert test_sup.hp_data_thread.is_alive()
    elif test_sup.dataflowtype == "string":
        assert test_sup.lp_data_thread._target == test_sup.listen_for_lp_string
        assert test_sup.lp_data_thread.is_alive()
        assert test_sup.hp_data_thread._target == test_sup.listen_for_hp_string
        assert test_sup.hp_data_thread.is_alive()
        
    assert test_sup.result_thread._target == test_sup.listen_for_result
    assert test_sup.result_thread.is_alive()
    
    keyboard_interrupt_simulation()
    
    #test_sup.stop_zmq()
    #del(test_sup)
        

def test_setup_result_channel():
    test_sup = Supervisor(file_path,'OOQS1')
    woker_man_test = WorkerManager(0,test_sup,"Generic")
    test_sup.setup_result_channel(woker_man_test,0)
    if woker_man_test.result_socket != "none":
        if woker_man_test.result_socket_type == "pushpull":
            assert test_sup.socket_result[0].type == zmq.SocketType.PUSH
        if woker_man_test.result_socket_type == "pubsub":
            assert test_sup.socket_result[0].type == zmq.SocketType.PUB
    # test_sup.stop_zmq()
    # #del(test_sup)

def test_start_managers():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.start_managers()
    assert test_sup.manager_workers != []
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_start_workers():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.start_workers()
    indexmanager=0
    for manager in test_sup.manager_workers: 
        if manager.processingtype == "thread":
            assert len(manager.worker_threads) == test_sup.manager_num_workers[indexmanager]
        if manager.processingtype == "process":
            assert len(manager.worker_processes) == test_sup.manager_num_workers[indexmanager]
        indexmanager = indexmanager + 1
    test_sup.stop_zmq()
    #del(test_sup)
        
def test_start():
    
    test_sup = Supervisor(file_path,'OOQS1')
    assert test_sup.status == "Initialised"
    my_thread = threading.Thread(target=test_sup.start)
    my_thread.start()
    
    time.sleep(2)
    
    assert test_sup.status == "Waiting"
    
    time.sleep(10)
    
    keyboard_interrupt_simulation()
    
    #assert test_sup.status == "Waiting"
    assert test_sup.status == "Shutdown"

    my_thread.join()
    
    test_sup.stop_zmq()
    #del(test_sup)

def test_handle_signal(capsys):
    test_sup = Supervisor(file_path,'OOQS1')
    possible_singals = [signal.SIGTERM, signal.SIGINT, random.choice(dir(signal))]
    random_signal = random.choice(possible_singals)
        
    captured_out = StringIO()
    sys.stdout = captured_out
    
    test_sup.handle_signals(random_signal, None)
    output = captured_out.getvalue().splitlines()
    sys.stdout = sys.__stdout__

    if random_signal == signal.SIGTERM:
        assert output[0].strip() == "SIGTERM received. Terminating with cleanedshutdown."
        assert test_sup.status == "Shutdown"
    elif random_signal == signal.SIGINT:
        assert output[0].strip() == "SIGINT received. Terminating with shutdown."
        assert test_sup.status == "Waiting"
    else:
        assert output[0].strip() == f"Received signal {random_signal}. Terminating."
        assert test_sup.status == "Waiting"
          
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_listen_for_result():
        
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.send_result = MagicMock()
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
      
    my_thread = threading.Thread(target=test_sup.listen_for_result())
    my_thread.start()
    
    time.sleep(1)
    
    test_sup.send_result.assert_called()
    
    time.sleep(1)
    
    keyboard_interrupt_simulation()
    
    test_sup.manager_workers[0].stop()
    my_thread.join()
            
    test_sup.stop_zmq()
    #del(test_sup)

def rcv_data(Sup,manager):
    context = zmq.Context()
    
    if Sup.datasockettype == "pubsub":
        publisher = context.socket(zmq.SUB)
        publisher.connect(manager.result_socket)
        publisher.setsockopt_string(zmq.SUBSCRIBE,"")
        time.sleep(1)
    elif Sup.datasockettype == "pushpull":
        publisher = context.socket(zmq.PULL)
        publisher.bind((manager.result_socket))
        #publisher.bind(get_pull_config(Sup.config.get(f"data_{lh}_socket")))
        time.sleep(1) 
    while not Sup.stopdata:
        if manager.result_dataflow_type == "string" or manager.result_dataflow_type == "filename":
            data = publisher.recv_string()
        elif manager.result_dataflow_type == "binary":
            data = publisher.recv()
            
    publisher.close()
    context.term()

    return data 

def test_send_result(): 
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup_2 = Supervisor(file_path,'OOQS2')
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    time.sleep(1)
    test_sup.manager_workers.append(WorkerManager(0,test_sup_2,"Generic"))    
    test_sup.manager_workers[1].start()
    time.sleep(1)
    
    test_sup.manager_workers[0].result_queue.put("test") 
    print(test_sup.manager_workers[0].result_queue)
    
    for index,manager in enumerate(test_sup.manager_workers):
        test_sup.setup_result_channel(manager,index)
    
    for index,manager in enumerate(test_sup.manager_workers):
        my_thread_2 = threading.Thread(target=test_sup.send_result(manager,index))
        my_thread_2.start()
        
        
        
        if manager.result_socket != "none":
            try:
                assert rcv_data(manager) == "test"
            except Exception:
                pass
        else:
            pass    
        
        my_thread_2.join()
        
    test_sup.stop_zmq()
    #del(test_sup)
       
#     captured_out = StringIO()
#     sys.stdout = captured_out
    
#     test_sup.send_result(fake_manager,0)
#     # captured = capsys.readouterr()
#     # assert captured.out.strip() == "test"
    
#     output = captured_out.getvalue().splitlines()
#     sys.stdout = sys.__stdout__
#     #try:
#     assert output[0].strip() == "test"
#     # except Exception: 
#     #     assert output[0].strip() == ""

def set_pub(Sup,lh):
            
    context = zmq.Context()
    
    if Sup.datasockettype == "pubsub":
        publisher = context.socket(zmq.PUB)
        publisher.bind(get_pull_config(Sup.config.get(f"data_{lh}_socket")))
        time.sleep(1)
    elif Sup.datasockettype == "pushpull":
        publisher = context.socket(zmq.PUSH)
        publisher.connect((Sup.config.get(f"data_{lh}_socket")))
        time.sleep(1)
    
    #print(publisher)
    return publisher
    
def send(Sup,publisher):        
    #while True:
        if Sup.dataflowtype == "binary" or Sup.dataflowtype == "filename":
            publisher.send(b"Hello, world!")
        elif Sup.dataflowtype == "string":
            publisher.send_string('Hello, world!')
             
    #publisher.close()
    #context.term()
def send_2(Sup,publisher):
    while not Sup.stopdata:
        #time.sleep(1)
        send(Sup,publisher)
        #time.sleep(1)    
    
def keyboard_interrupt_simulation():
    os.kill(os.getpid(), signal.SIGINT)

def test_listen_for_data():
        
    test_sup = Supervisor(file_path,'OOQS1')
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()  
    
    time.sleep(1)
    
    priority = random.choice(['lp','hp'])
    
    publisher = set_pub(test_sup,priority)
    
    my_thread_2 = threading.Thread(target=send_2, args=(test_sup, publisher))
    
    # my_thread = threading.Thread(target=test_sup.listen_for_data,args=(priority,))
    # my_thread.start()
    
    # print(test_sup.is_listening)
    
    # time.sleep(1)
    
    # wait_at_most(5).poll_interval(1).with_description("Supervisor is listening for commands").until(lambda:test_sup.is_listening)

    my_thread_2.start()
    
    # while not test_sup.stopdata:
        
    #     send(test_sup,publisher)
        
    time.sleep(1)                 
    
    # my_thread = threading.Thread(target=test_sup.listen_for_data,args=(priority,))
    # my_thread.start()
    
    #if not test_sup.stopdata:
    test_sup.listen_for_data(priority)
    #try:   
    if priority == 'lp':    
        assert test_sup.manager_workers[0].low_priority_queue.get() == "Hello, world!"
    elif priority == 'hp':
        assert test_sup.manager_workers[0].high_priority_queue.get() == "Hello, world!"
        # except zmq.error.Again:
            #     pass  
                 
    print(test_sup.stopdata)
    
    time.sleep(1)

    keyboard_interrupt_simulation()

    print(test_sup.stopdata)
    
    time.sleep(1)
    
    #my_thread.join()
    my_thread_2.join()
    #publisher.close()
    
def test_listen_for_hp_data():
        
    test_sup = Supervisor(file_path,'OOQS1')
        
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()  
    
    send(test_sup,'hp')
        
    my_thread = threading.Thread(target=test_sup.listen_for_hp_data)
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.manager_workers[0].high_priority_queue.get() == b"Hello, world!"
    
    #keyboard_interrupt_simulation()

    test_sup.manager_workers[0].stop()
    my_thread.join()
    
    test_sup.stop_zmq()
    #del(test_sup)

def test_listen_for_lp_string():
            
    test_sup = Supervisor(file_path,'OOQS1')
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()  
    
    send(test_sup,'lp')
        
    my_thread = threading.Thread(target=test_sup.listen_for_lp_string)
    my_thread.start()
    
    time.sleep(1)
        
    assert test_sup.manager_workers[0].low_priority_queue.get() == 'Hello, world!'
    
    keyboard_interrupt_simulation()

    test_sup.manager_workers[0].stop()
    my_thread.join()
    
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_listen_for_hp_string():
            
    test_sup = Supervisor(file_path,'OOQS1')
        
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()  
    
    send(test_sup,'hp')
        
    my_thread = threading.Thread(target=test_sup.listen_for_hp_string)
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.manager_workers[0].high_priority_queue.get() == "Hello, world!"
    
    keyboard_interrupt_simulation()

    test_sup.manager_workers[0].stop()
    my_thread.join()

    test_sup.stop_zmq()
    #del(test_sup)

def test_listen_for_lp_file():
                
    test_sup = Supervisor(file_path,'OOQS1')
        
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()  
    
    send(test_sup,'lp')
        
    my_thread = threading.Thread(target=test_sup.listen_for_lp_file)
    my_thread.start()
        
    time.sleep(1)
    
    assert test_sup.manager_workers[0].low_priority_queue.get() == b"Hello, world!"
    
    keyboard_interrupt_simulation()

    test_sup.manager_workers[0].stop()
    my_thread.join()
    
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_listen_for_hp_file():
                
    test_sup = Supervisor(file_path,'OOQS1')
        
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()  
    
    send(test_sup,'hp')
        
    my_thread = threading.Thread(target=test_sup.listen_for_hp_file)
    my_thread.start()
        
    time.sleep(1)
    
    assert test_sup.manager_workers[0].high_priority_queue.get() == b"Hello, world!"
    
    keyboard_interrupt_simulation()

    test_sup.manager_workers[0].stop()
    my_thread.join()
    
    test_sup.stop_zmq()
    #del(test_sup)

def test_listen_for_commands():
    test_sup = Supervisor(file_path,'OOQS1')
    
    context = zmq.Context()
    publisher = context.socket(zmq.PUB)
    publisher.bind(get_pull_config(test_sup.config.get("command_socket")))
    command_data = {
    "header": {
        "subtypeNone": "subtype",
        "pidtarget": "pidtarget",
        "pidsource": "valore_pidsource"
        },   
    }   
    
    #captured_out = StringIO()
    #sys.stdout = captured_out
    
    command_json_string = json.dumps(command_data)
    
    time.sleep(1)
    
    publisher.send_string(command_json_string)

    time.sleep(1)

    my_thread = threading.Thread(target=test_sup.listen_for_commands)
    my_thread.start()
    
    time.sleep(1)
    
    #output = captured_out.getvalue().splitlines()
    #sys.stdout = sys.__stdout__
    
    #assert output[0].strip == "Waiting for commands..."
    #assert output[1].strip == "{'header': {'subtypeNone': 'subtype', 'pidtarget': 'pidtarget', 'pidsource': 'valore_pidsource'},}"
        
    my_thread.join(2)
    
    publisher.close()
    print(publisher)
    #context.term()
    test_sup.stop_zmq()
    print("stop")
    #del(test_sup)

def test_command_shutdown():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.command_stopdata = MagicMock()
    test_sup.command_stop = MagicMock()
    test_sup.stop_all = MagicMock()
    
    test_sup.command_shutdown()
    assert test_sup.status == "Shutdown"
    test_sup.command_stopdata.assert_called()
    test_sup.command_stop.assert_called()    
    test_sup.stop_all.assert_called_once()      
    assert test_sup.continueall == False  

    test_sup.stop_zmq()
    #del(test_sup)        
    
def test_command_cleanandshutdown():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.command_stopdata = MagicMock()
    test_sup.stop_all = MagicMock()
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    assert test_sup.status == "Initialised"

    test_sup.change_status()
    
    time.sleep(1)
    
    my_thread = threading.Thread(target=test_sup.command_cleanedshutdown)
    my_thread.start()
    
    time.sleep(1)
    
    if test_sup.status == "Processing":
        time.sleep(1)
        assert test_sup.status == "EndingProcessing"
        time.sleep(1)
        test_sup.command_stopdata.assert_called_once()
        for manager in test_sup.manager_workers:
            assert manager.status == "EndingProcessing"
            time.sleep(1)
            assert manager.status == "Shutdown"
    else: 
        time.sleep(1)
        test_sup.stop_all.assert_called_once()
        assert test_sup.continueall == False
        assert test_sup.status == "Shutdown"
        
    my_thread.join()
    
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_command_reset():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.command_stopdata = MagicMock()
    test_sup.command_stop = MagicMock()
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    for manager in test_sup.manager_workers:
        manager.clean_queue = MagicMock()
        
    test_sup.change_status()    
    
    my_thread = threading.Thread(target=test_sup.command_reset)
    my_thread.start()
    
    time.sleep(1)    
    
    if test_sup.status == "Processing" or test_sup.status == "Waiting":
        test_sup.command_stopdata.assert_called_once()
        test_sup.command_stop.assert_called_once()
        for manager in test_sup.manager_workers:
            manager.clean_queue.assert_called()
        assert test_sup.status == "Waiting"
    else:
        pass
    
    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)
        
def test_command_start():
    test_sup = Supervisor(file_path,'OOQS1')   
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    my_thread = threading.Thread(target=test_sup.command_start)
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.status == "Processing"
    for manager in test_sup.manager_workers:
        assert manager.status == "Processing"

    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_command_stop():
    test_sup = Supervisor(file_path,'OOQS1')   
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    my_thread = threading.Thread(target=test_sup.command_stop)
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.status == "Waiting"
    for manager in test_sup.manager_workers:
        assert manager.status == "Waiting"

    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_command_startdata():
    test_sup = Supervisor(file_path,'OOQS1')   
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    my_thread = threading.Thread(target=test_sup.command_startdata)
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.stopdata == False
    for manager in test_sup.manager_workers:
        assert manager.stopdata == False

    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)

def test_command_stopdata():
    test_sup = Supervisor(file_path,'OOQS1')   
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    my_thread = threading.Thread(target=test_sup.command_stopdata)
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.stopdata == True
    for manager in test_sup.manager_workers:
        assert manager.stopdata == True

    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)
    
def test_process_command():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.command_cleanedshutdown = MagicMock() # questi due comandi vengono inizializzati come MagicMock per andarne
    test_sup.command_shutdown = MagicMock() # a controllare l'effetiva chiamata, poichè questi cambiano per due volte il self.status
    
    possible_subtype = ['shutdown','cleanandshutdown','getstatus','start','stop','reset','stopdata','startdata']
    random_subtype = random.choice(possible_subtype)
    possible_pidtarget = ['*',f'{test_sup.name}','all','All','aLl','alL','ALl','ALL','aLL','AlL','others']
    random_pidtarget = random.choice(possible_pidtarget)
    
    command_data = {
    "header": {
        "subtypeNone": f"{random_subtype}",
        "pidtarget": f"{random_pidtarget}",
        "pidsource": "valore_pidsource"
        }   
    }   
    
    command_json_string = json.dumps(command_data)
    command = json.loads(command_json_string)
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    for manager in test_sup.manager_workers:
        manager.monitoring_thread.sendto = MagicMock()
    
    my_thread = threading.Thread(target=test_sup.process_command(command))
    my_thread.start()

    time.sleep(1)
    
    if command['header']['pidtarget'] == test_sup.name or command['header']['pidtarget'] == "all".lower() or command['header']['pidtarget'] == "*":
        if command['header']['subtypeNone'] == "shutdown":
            test_sup.command_shutdown.assert_called_once()
        if command['header']['subtypeNone'] == "cleanedshutdown":
            test_sup.command_cleanedshutdown.assert_called_once()
        if command['header']['subtypeNone'] == "getstatus":
            for manager in test_sup.manager_workers:
                manager.monitoring_thread.sendto.assert_called()
        if command['header']['subtypeNone'] == "start":
            assert test_sup.status == "Processing"
            for manager in test_sup.manager_workers:
                assert manager.status == "Processing"
        if command['header']['subtypeNone'] == "stop":
            assert test_sup.status == "Waiting"
            for manager in test_sup.manager_workers:
                assert manager.status == "Waiting"
        if command['header']['subtypeNone'] == "reset":
            assert test_sup.status == "Processing"
            for manager in test_sup.manager_workers:
                assert manager.status == "Processing"
        if command['header']['subtypeNone'] == "stopdata":
            assert test_sup.stopdata == True
            for manager in test_sup.manager_workers:
                assert manager.stopdata == True
        if command['header']['subtypeNone'] == "startdata":
            assert test_sup.stopdata == False
            for manager in test_sup.manager_workers:
                assert manager.stopdata == False
    else:
        print("None of one of the expected name was found")
    
    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)        
            
def test_stop_all():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.command_stop = MagicMock()
    
    test_sup.manager_workers.append(WorkerManager(0,test_sup,"Generic"))    
    test_sup.manager_workers[0].start()
    
    time.sleep(1)
    
    assert test_sup.status == "Initialised"
    
    my_thread = threading.Thread(target=test_sup.stop_all())
    my_thread.start()
    
    time.sleep(1)
    
    assert test_sup.stopdata == True
    for manager in test_sup.manager_workers:
        assert manager.stopdata == True
        
    test_sup.command_stop.assert_called_once()
        
    # assert test_sup.status == "Waiting"
    # for manager in test_sup.manager_workers:
    #     assert manager.status == "Waiting"
    
    time.sleep(0.1)
    
    for managers in test_sup.manager_workers:
        if manager.processingtype == "process":
            assert manager._stop_event.is_set()
            #if manager._stop_event.set():
            assert manager.status == "End"
            #manager.stop.assert_called_once_with(False)
        else:
            assert manager._stop_event.is_set()
            assert manager.status == "End"
            #manager.stop.assert_called_once_with(False)
        #manager.join.assert_called_once()
    
    my_thread.join()
    test_sup.manager_workers[0].stop()
    test_sup.stop_zmq()
    #del(test_sup)
    

    
    
 
             

    

    

    
    
         
    


        
            
        
        
