#!/bin/bash

SCRIPT_DIR=$(dirname "$(realpath "$0")")
# Function to display usage information
usage() {
    echo "Usage: $0 -s [Avro|PB|CCSDS] [-n <number>] [-l <log_dir>]"
    exit 1
}

# Parse arguments
while getopts ":s:n:l:" opt; do
  case $opt in
    s)
      SCHEMA=$OPTARG
      ;;
    n)
      NUMBER=$OPTARG
      ;;
    l)
      LOG_DIR=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      usage
      ;;
  esac
done

# Check that -s was provided and has a valid value
if [ -z "$SCHEMA" ]; then
    echo "Error: -s option is mandatory."
    usage
fi

if [[ "$SCHEMA" != "Avro" && "$SCHEMA" != "PB" && "$SCHEMA" != "CCSDS" ]]; then
    echo "Error: -s option must be one of [Avro, PB, CCSDS]."
    usage
fi

# Set default values if not provided
LOG_DIR=${LOG_DIR:-"${SCRIPT_DIR}/logs"}

# Create the log directory with the current datetime appended
DATETIME=$(date +"%Y%m%d_%H%M%S")
LOG_DIR="${LOG_DIR}/${SCHEMA}_N_${NUMBER}_${DATETIME}"
mkdir -p "$LOG_DIR"

# Define program names based on the -s value
CONSUMER_PROGRAM="./${SCHEMA}CppCons"
PRODUCER_PROGRAM="./${SCHEMA}CppProd"

# Change to the build directory
cd build || { echo "Failed to change directory to build. Exiting."; exit 1; }

# Start the consumer program in the background, redirecting stdout and stderr to log files
$CONSUMER_PROGRAM $NUMBER > "${LOG_DIR}/${CONSUMER_PROGRAM}.log" 2>&1 &
CONSUMER_PID=$!

# Start the producer program in the background, redirecting stdout and stderr to log files
$PRODUCER_PROGRAM $NUMBER > "${LOG_DIR}/${PRODUCER_PROGRAM}.log" 2>&1 &
PRODUCER_PID=$!

# Wait for both programs to complete
wait $CONSUMER_PID
wait $PRODUCER_PID

echo "Both programs have completed. Logs can be found in: $LOG_DIR"
