#!/bin/bash

base_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
logs_path=$base_path/logs

if [ -f $ACSDATA/tmp/acsInstance0.lock ]
then
    printf "\n\33[32m Stopping ACS.. \33[0m\n\n"
    acsStop > "$logs_path/acsStop.log" 2>&1
fi

printf "\n\33[32m Starting ACS.. \33[0m\n\n"
acsStart > "$logs_path/acsStart.log" 2>&1

./start_acs_containers.sh