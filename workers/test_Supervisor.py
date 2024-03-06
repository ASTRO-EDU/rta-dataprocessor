from MonitoringPoint import MonitoringPoint
from WorkerThread import WorkerThread
from MonitoringThread import MonitoringThread
from WorkerManager import WorkerManager
from WorkerProcess import WorkerProcess
from ConfigurationManager import ConfigurationManager, get_pull_config
from Supervisor import Supervisor
from unittest.mock import MagicMock
#import pytest_mock
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

file_path = '/home/nunzio/rta-dataprocessor/config.json'
file_path_2 = '/home/nunzio/rta-dataprocessor/config_2.json'

@pytest.fixture
# def cleanup_after_test(request):
#     def cleanup():
#         test_sup.stop_zmq()
#         print("\nEseguito cleanup dopo il test")
        
#     request.addfinalizer(cleanup)
        

def test_init_pubsub():
    test_sup = Supervisor(file_path,'OOQS1')
    
    assert test_sup.name == 'OOQS1'
    #assert test_sup.config_manager == ConfigurationManager(file_path)
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
    del(test_sup)

    
def test_init_pushpull():
    test_sup = Supervisor(file_path,'OOQS2')

    assert test_sup.name == 'OOQS2'
    #assert test_sup.config_manager == ConfigurationManager(file_path)
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
    del(test_sup)


def test_Supervisor_with_different_datasockettype():
    with pytest.raises(ValueError) as raise_test:
        Supervisor(file_path_2,'OOQS3')
    assert str(raise_test.value) == "Config file: datasockettype must be pushpull or pubsub"
    
def test_start_service_threads():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.start_service_threads()
    
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
    test_sup.stop_zmq()
    del(test_sup)
        

def test_setup_result_channel():
    test_sup = Supervisor(file_path,'OOQS1')
    woker_man_test = WorkerManager(0,test_sup,"Generic")
    test_sup.setup_result_channel(woker_man_test,0)
    if woker_man_test.result_socket != "none":
        if woker_man_test.result_socket_type == "pushpull":
            assert test_sup.socket_result[0].type == zmq.SocketType.PUSH
        if woker_man_test.result_socket_type == "pubsub":
            assert test_sup.socket_result[0].type == zmq.SocketType.PUB
    test_sup.stop_zmq()
    del(test_sup)

def test_start_managers():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.start_managers()
    assert test_sup.manager_workers != []
    test_sup.stop_zmq()
    del(test_sup)
    
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
    del(test_sup)
        
# def mock_get(mocker):
#     mock = Mock()
#     mocker.patch('test_sup.start', return_value = mock)
#     return mock

def test_start():
    test_sup = Supervisor(file_path,'OOQS1')
    
    #mock_get.test_sup.start.assert_called()
    
    # test_sup.start = MagicMock()
    # time.sleep = MagicMock()
    
    test_sup.start()
    
    
    # time.sleep.assert_called_once_with(1)
    try:
        assert test_sup.status == "Waiting"
    except KeyboardInterrupt:
        assert test_sup.status == "Shutdown"
        
    
    
    
    #test_sup.start_service_threads.assert_called_once()
    #test_sup.start_managers.assert_called_once()
    #test_sup.start_workers.assert_called_once()
    
    # try:
    #     test_sup.listen_for_commands.assert_called_once()
    #     # test_sup.time.sleep.assert_called_with(1)
    # except KeyboardInterrupt:
    #     #test_sup.print().assert_called_with("Keyboard interrupt received. Terminating.")
    #     test_sup.command_shutdown().assert_called_once()
    test_sup.stop_zmq()
    del(test_sup)

def test_handle_signal(capsys):
    test_sup = Supervisor(file_path,'OOQS1')
    #possible_singals = [signal.SIGTERM, signal.SIGINT, None]
    #random_signal = random.choice(possible_singals)
    signal_ = signal.SIGTERM
    assert test_sup.status == "Initialised"
    test_sup.handle_signals(signal_, None)
    assert test_sup.status == "Shutdown"
    test_sup.stop_zmq()
    del(test_sup)
    # for manager in test_sup.manager_workers:
    #     assert manager.status == "Waiting"
    
    
    # capture = capsys.readouterr()    
    # if signal_ == signal.SIGTERM:
    #     assert capture.out == "SIGTERM received. Terminating with cleanedshutdown.\n"
    # elif signal_ == signal.SIGINT:
    #     assert capture.out == "SIGINT received. Terminating with shutdown.\n"
    # else: 
    #     assert capture.out == "Received signal {signal_}. Terminating.\n" 
    
def test_listen_for_result():
    test_sup = Supervisor(file_path,'OOQS1')
    test_sup.send_result = MagicMock()
    test_sup.listen_for_result()
    for manager in test_sup.manager_workers:
            test_sup.send_result.assert_called_once()
    test_sup.stop_zmq()
    del(test_sup)

# def test_send_result(): 
#     test_sup = Supervisor(file_path,'OOQS1')
#     for i in range(len(test_sup.manager_workers)):
#         test_sup.send_result(test_sup.manager_workers[i],i)
#         test = (random[queue.Empty, Exception, None])

@pytest.fixture
def mock_socket_lp_data():
    return MagicMock()

@pytest.fixture
def mock_manager():
    return MagicMock()

def test_listen_for_lp_data(mock_socket_lp_data, mock_manager):
    test_sup = Supervisor(file_path,'OOQS1')
    #test_sup.stopdata = random[True,False]
    
    if test_sup.stopdata == True:
        test_sup.listen_for_lp_data()
        mock_socket_lp_data.recv.assert_not_called()
        mock_manager.low_priority_queue.put.assert_not_called()
    elif test_sup.stopdata == False:
        mock_socket_lp_data.recv.side_effect = ['data1', 'data2', 'data3']
        test_sup.decode_data = MagicMock(return_value='decoded_data')
        test_sup.listen_for_lp_data()
        mock_socket_lp_data.recv.assert_called()
        test_sup.decode_data.assert_called()
        for manager in test_sup.manager_workers:
            manager.low_priority_queue.put.assert_called_with('decoded_data')
        
        
            
        
        
