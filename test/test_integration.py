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
import zmq
from unittest.runner import TextTestResult

# Setup base paths relative to this script
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
WORKSPACE_ROOT = PROJECT_ROOT  # Assuming the workspace root is the project root
CPP_DIR = WORKSPACE_ROOT / "c++"
WORKERS_DIR = WORKSPACE_ROOT / "workers"
TEST_DIR = WORKSPACE_ROOT / "test"
LOGS_DIR = WORKSPACE_ROOT / "logs"

logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s: %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)
logger = logging.getLogger(__name__)

class GracefulTestResult(TextTestResult):
    def addError(self, test, err):
        exc_type = err[0]
        if exc_type == KeyboardInterrupt:
            self.stream.writeln()
            self.stream.writeln("Test interrupted by user")
            return
        super().addError(test, err)

class TestIntegration(unittest.TestCase):
    def setUp(self):
        logger.info('Setting up integration test environment')
        # Get the RTACONFIG environment variable or use default
        self.rtaconfig = os.environ.get('RTACONFIG', str(CPP_DIR / 'config.json'))
        
        # Store process handles
        self.processes = []
        
        # Create a flag for graceful shutdown
        self.should_stop = False

        # Set up signal handlers
        self.original_sigint = signal.getsignal(signal.SIGINT)
        self.original_sigterm = signal.getsignal(signal.SIGTERM)
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
    
    def signal_handler(self, signum, frame):
        if hasattr(self, '_handling_signal'):
            logger.info('Already handling signal, skipping...')
            return
        self._handling_signal = True
        
        logger.info(f'Received signal {signum}, initiating graceful shutdown...')
        self.should_stop = True
        
        try:
            self.cleanup()
        finally:
            # Restore original signal handlers
            signal.signal(signal.SIGINT, self.original_sigint)
            signal.signal(signal.SIGTERM, self.original_sigterm)
            # Use KeyboardInterrupt instead of GracefulExit
            raise KeyboardInterrupt()

    def cleanup(self):
        """Separate cleanup method to handle process termination in specific order"""
        if hasattr(self, '_is_cleaning_up'):
            return
        self._is_cleaning_up = True
        
        logger.info('Starting sequential cleanup of processes...')
        
        def terminate_process(process, name, timeout=3):
            if not process or process.poll() is not None:
                return

            try:
                logger.info(f'Sending SIGINT to {name} (PID: {process.pid})')
                process.send_signal(signal.SIGINT)
                
                # Poll for process termination
                start_time = time.time()
                while time.time() - start_time < timeout:
                    if process.poll() is not None:
                        logger.info(f'{name} terminated successfully')
                        return
                    time.sleep(0.1)
                
                # If still running, try SIGTERM
                if process.poll() is None:
                    logger.warning(f'{name} did not respond to SIGINT, sending SIGTERM')
                    process.terminate()
                    
                    # Wait again
                    start_time = time.time()
                    while time.time() - start_time < 2:
                        if process.poll() is not None:
                            logger.info(f'{name} terminated with SIGTERM')
                            return
                        time.sleep(0.1)
                    
                    # Last resort: SIGKILL
                    if process.poll() is None:
                        logger.error(f'{name} not responding, sending SIGKILL')
                        process.kill()
                        process.wait(timeout=1)
            except Exception as e:
                logger.error(f'Error while terminating {name} (PID: {process.pid}): {e}')
                try:
                    if process.poll() is None:
                        process.kill()
                        process.wait(timeout=1)
                except:
                    pass

        # Find processes by their command
        consumer_process = None
        simulator_process = None
        monitoring_process = None
        other_processes = []

        for process in self.processes:
            if not process or process.poll() is not None:
                continue
                
            if hasattr(process, 'args'):
                cmd = ' '.join(process.args)
                if 'ProcessDataConsumer1' in cmd:
                    consumer_process = process
                elif 'gfse.py' in cmd:
                    simulator_process = process
                elif 'ProcessMonitoring.py' in cmd:
                    monitoring_process = process
                else:
                    other_processes.append(process)

        # Step 1: Stop the consumer first with extended timeout
        if consumer_process:
            terminate_process(consumer_process, "Consumer", timeout=15)  # Increased timeout
            time.sleep(5)  # Reduced post-termination wait

        # Step 2: Stop the DAMS simulator
        if simulator_process:
            terminate_process(simulator_process, "DAMS Simulator")
            time.sleep(2)

        # Step 3: Stop the monitoring process
        if monitoring_process:
            terminate_process(monitoring_process, "Process Monitoring")
            time.sleep(1)

        # Step 4: Stop any remaining processes
        for process in other_processes:
            if process and process.poll() is None:
                terminate_process(process, f"Other Process (PID: {process.pid})")

        logger.info('Sequential cleanup completed')

    def tearDown(self):
        logger.info('Tearing down integration test environment')
        self.cleanup()

    def run_process_monitoring(self):
        logger.info('Starting ProcessMonitoring')
        os.makedirs(LOGS_DIR, exist_ok=True)
        log_file = open(LOGS_DIR / 'monitoring.log', 'w')
        cmd = ['python3', 'ProcessMonitoring.py', self.rtaconfig]
        process = subprocess.Popen(
            cmd,
            cwd=str(WORKERS_DIR),
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
            cwd=str(CPP_DIR / 'build'),
            stdout=None,
            stderr=None
        )
        self.processes.append(process)
        logger.info(f'Consumer started with PID {process.pid}')

        return process

    def run_dams_simulator(self, addr, port, indir, rpid, wform_sec, restart=True):
        logger.info('Starting DAMS simulator')
        
        # Set restart to True to enable continuous processing
        restart = True  # Enable/DISABLE restart. Modify this line to change behavior
        
        cmd = [
            'python3', str(TEST_DIR / 'gfse.py'),
            '--addr', addr,
            '--port', str(port),
            '--indir', indir,
            '--rpid', str(rpid),
            '--wform-sec', str(wform_sec)
        ]
        
        # Add restart flag if requested
        if restart:
            cmd.append('--restart')
            logger.info('DAMS simulator configured to restart when finished')
        
        # Give the simulator time to initialize
        # time.sleep(8)

        process = subprocess.Popen(
            cmd,
            cwd=str(TEST_DIR / 'dl0_simulated'),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        self.processes.append(process)
        logger.info(f'DAMS simulator started with PID {process.pid}')
        
        return process

    def send_start_command(self):
        logger.info('Sending start command to system components')
        # This command sends the start command to the main system components
        cmd = ['python3', 'SendCommand.py', self.rtaconfig, 'start', 'all']
        process = subprocess.Popen(
            cmd,
            cwd=str(WORKERS_DIR),
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

        # Start DAMS simulator
        simulator_process = self.run_dams_simulator(
            addr='127.0.0.1',
            port=1234,
            indir=str(TEST_DIR / 'dl0_simulated'),
            rpid=1,
            wform_sec=200
        )
        self.assertIsNotNone(simulator_process, "Failed to start DAMS simulator")
        time.sleep(6)  # Give it time to initialize

        # Start the Consumer
        consumer_process = self.run_consumer()
        self.assertIsNotNone(consumer_process, "Failed to start Consumer")
        # time.sleep(2)  # Give it time to initialize

        # Wait before sending the start command to system components
        time.sleep(3)
        # Send start command to system components
        start_process = self.send_start_command()
        self.assertIsNotNone(start_process, "Failed to send start command")
        
        # Wait for some time to let the system process data
        logger.info('Waiting for system to process data (100)')
        time.sleep(100)  # Adjust this time based on your needs

        # Check if processes are still running
        logger.info('Checking if main processes are still running')
        self.assertEqual(monitoring_process.poll(), None, "ProcessMonitoring died unexpectedly")
        self.assertEqual(consumer_process.poll(), None, "Consumer died unexpectedly")
        self.assertEqual(simulator_process.poll(), None, "DAMS simulator died unexpectedly")
        logger.info('Integration test completed successfully')

def main():
    suite = unittest.TestLoader().loadTestsFromTestCase(TestIntegration)
    runner = unittest.TextTestRunner(resultclass=GracefulTestResult)
    try:
        result = runner.run(suite)
        sys.exit(not result.wasSuccessful())
    except KeyboardInterrupt:
        logger.info("Test interrupted by user, exiting cleanly")
        sys.exit(0)

if __name__ == '__main__':
    main() 