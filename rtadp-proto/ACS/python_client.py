import sys
import os
import subprocess
import time

import ACS

from datetime import datetime
import logging

from Acspy.Clients.SimpleClient import PySimpleClient
__original_logger_class = logging.getLoggerClass()

logging.setLoggerClass(__original_logger_class)
client = PySimpleClient()

if __name__ == "__main__":
    # Replace 'your_file.txt' with the actual filename
    print("\nWelcome to the OOQS Command Line Interface\n")
