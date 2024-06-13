from ConfigurationManager import ConfigurationManager
from ConfigurationManager import get_pull_config
import json
import pytest

def test_read_configuration_with_a_json_file():
    test_address = "tcp://localhost:5555"
    file_path = 'test_configuration_manager.json'
    configuration_example = ConfigurationManager(file_path)
    expected_address = "tcp://*:5555"
    expected_configuration = [{'processname': 'A', 'dataflow_type': '0', 'manager_result_socket_type': 'a', 'manager_result_dataflow_type': 'b', 'manager_result_socket': 'c', 'manager_num_workers': '3'}, {'processname': 'B', 'dataflow_type': '1', 'manager_result_socket_type': 'd', 'manager_result_dataflow_type': 'e', 'manager_num_workers': '4'}]
    expected_config = {'A': {'processname': 'A', 'dataflow_type': '0', 'manager_result_socket_type': 'a', 'manager_result_dataflow_type': 'b', 'manager_result_socket': 'c', 'manager_num_workers': '3'}, 'B': {'processname': 'B', 'dataflow_type': '1', 'manager_result_socket_type': 'd', 'manager_result_dataflow_type': 'e', 'manager_num_workers': '4'}}
    expected_worker_config = {'A':('a', 'b', 'c', '3'), 'B':('d', 'e', [], '4')}
    
    # test get_pull_config
    assert get_pull_config(test_address) == expected_address
    # test read_configurations_from_file
    assert configuration_example.configurations == expected_configuration
    # test create_memory_structure
    assert configuration_example.config == expected_config
    # test get_configuration
    for config in configuration_example.config.keys():
        assert configuration_example.get_configuration(config) == expected_config.get(config)
    # test get_workers_config
    for config in configuration_example.config.keys():
        assert configuration_example.get_workers_config(config) == expected_worker_config.get(config)
    #test validate_configurations
    try:
        configuration_example.validate_configurations(configuration_example.configurations)
        raise AssertionError
    except ValueError:
        pass
    

def test_read_configuration_with_an_inexistent_json_file():
    file_path = 'Inexistent_fil.json'
    with pytest.raises(FileNotFoundError):
        configuration_example = ConfigurationManager(file_path)

def test_read_configuration_with_an_incorrect_json_file():
    file_path = 'Incorrect_file.json'
    with pytest.raises(json.JSONDecodeError):
        configuration_example = ConfigurationManager(file_path)