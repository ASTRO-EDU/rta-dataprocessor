#!/usr/bin/env bash

# Check if the argument is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <local|distributed>"
    exit 1
fi

# Get the argument
mode=$1

base_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
logs_path=$ACSDATA/logs
mkdir -p $logs_path/acscont1/
mkdir -p $logs_path/acscont2/
printf "\n\33[32m Starting containers.. \33[0m\n\n"
printf "ACS_CDB=$ACS_CDB\n"

acsStopContainer pyContainer1 2>&1
acsStopContainer pyContainer2 2>&1

if [ "$mode" = "local" ]; then
    # Start containers locally
    nohup acsStartContainer -py --name pyContainer1 > "$logs_path/acscont1/pyContainer1.log" &
    nohup acsStartContainer -py --name pyContainer2 > "$logs_path/acscont2/pyContainer2.log" &
elif [ "$mode" = "distributed" ]; then
    # Start containers distributed
    nohup acsStartContainer -py --name pyContainer1 --remoteHost acscont1 -m corbaloc::acsmanager:3000/Manager > "$logs_path/acscont1/pyContainer1.log" &
    nohup acsStartContainer -py --name pyContainer2 --remoteHost acscont2 -m corbaloc::acsmanager:3000/Manager > "$logs_path/acscont2/pyContainer2.log" &
else
    echo "Invalid mode specified. Use 'local' or 'distributed'."
    exit 1
fi
