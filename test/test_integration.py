#!/usr/bin/env python3

import os
import sys
import time
import signal
import subprocess
import threading
import unittest
from pathlib import Path
import logging

logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

class TestIntegration(unittest.TestCase):
    def setUp(self):
        logger.info('Setting up integration test environment')
        # Get the RTACONFIG environment variable or use default
        self.rtaconfig = os.environ.get('RTACONFIG', '/home/worker/workspace/c++/config.json')
        
        # Store process handles
        self.processes = []
        
        # Create a flag for graceful shutdown
        self.should_stop = False

    def tearDown(self):
        logger.info('Tearing down integration test environment')
        # Set the stop flag
        self.should_stop = True
        
        # Kill all processes
        for process in self.processes:
            if process.poll() is None:  # If process is still running
                process.terminate()
                process.wait()

    def run_process_monitoring(self):
        logger.info('Starting ProcessMonitoring')
        os.makedirs('/home/worker/logs', exist_ok=True)
        log_file = open('/home/worker/logs/monitoring.log', 'w')
        cmd = ['python3', 'ProcessMonitoring.py', self.rtaconfig]
        process = subprocess.Popen(
            cmd,
            cwd='/home/worker/workspace/workers',
            stdout=log_file,
            stderr=log_file
        )
        self.processes.append(process)
        logger.info(f'ProcessMonitoring started with PID {process.pid}')
        return process

    def run_consumer(self):
        logger.info('Starting C++ Consumer')
        cmd = ['./ProcessDataConsumer1', self.rtaconfig]
        process = subprocess.Popen(
            cmd,
            cwd='/home/worker/workspace/c++/build',
            stdout=None,
            stderr=None
        )
        self.processes.append(process)
        logger.info(f'Consumer started with PID {process.pid}')
        return process

    def run_dams_simulator(self, addr, port, indir, rpid, wform_sec):
        logger.info('Starting DAMS simulator')
        cmd = [
            'python3', 'gfse.py',
            '--addr', addr,
            '--port', str(port),
            '--indir', indir,
            '--rpid', str(rpid),
            '--wform-sec', str(wform_sec)
        ]
        process = subprocess.Popen(
            cmd,
            cwd='/home/worker/workspace/dl0_simulated',
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        self.processes.append(process)
        logger.info(f'DAMS simulator started with PID {process.pid}')
        return process

    def send_start_command(self):
        logger.info('Sending start command')
        cmd = ['python3', 'SendCommand.py', self.rtaconfig, 'start', 'all']
        process = subprocess.Popen(
            cmd,
            cwd='/home/worker/workspace/workers',
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        self.processes.append(process)
        stdout, stderr = process.communicate()
        logger.info(f'SendCommand output: {stdout.decode().strip()}')
        if stderr:
            logger.error(f'SendCommand error: {stderr.decode().strip()}')
        return process

    def test_full_integration(self):
        logger.info('Starting full integration test')
        # Start ProcessMonitoring
        monitoring_process = self.run_process_monitoring()
        self.assertIsNotNone(monitoring_process, "Failed to start ProcessMonitoring")
        time.sleep(2)  # Give it time to initialize

        # Start the Consumer
        consumer_process = self.run_consumer()
        self.assertIsNotNone(consumer_process, "Failed to start Consumer")
        time.sleep(2)  # Give it time to initialize

        # Start DAMS simulator
        simulator_process = self.run_dams_simulator(
            addr='127.0.0.1',
            port=1234,
            indir='/home/worker/workspace/dl0_simulated',
            rpid=1,
            wform_sec=100
        )
        self.assertIsNotNone(simulator_process, "Failed to start DAMS simulator")
        time.sleep(2)  # Give it time to initialize

        # Wait before sending the start command
        time.sleep(5)
        # Send start command
        start_process = self.send_start_command()
        self.assertIsNotNone(start_process, "Failed to send start command")
        
        # Wait for some time to let the system process data
        logger.info('Waiting for system to process data (30s)')
        time.sleep(30)  # Adjust this time based on your needs

        # Check if processes are still running
        logger.info('Checking if main processes are still running')
        self.assertEqual(monitoring_process.poll(), None, "ProcessMonitoring died unexpectedly")
        self.assertEqual(consumer_process.poll(), None, "Consumer died unexpectedly")
        self.assertEqual(simulator_process.poll(), None, "DAMS simulator died unexpectedly")
        logger.info('Integration test completed successfully')

if __name__ == '__main__':
    unittest.main() 