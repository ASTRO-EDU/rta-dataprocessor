#!/bin/bash

# Activate conda environment
source /opt/conda/bin/activate worker

# Set up development environment
cd /home/worker/workspace

# If no arguments are provided, start an interactive shell
if [ $# -eq 0 ]; then
    exec /bin/bash
else
    # Execute the provided command
    exec "$@"
fi
