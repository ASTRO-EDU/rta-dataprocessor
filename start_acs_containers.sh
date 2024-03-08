#!/usr/bin/env bash

base_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
logs_path=$base_path/logs

printf "\n\33[32m Starting containers.. \33[0m\n\n"
printf "ACS_CDB=$ACS_CDB\n"

acsStopContainer pyContainer1 2>&1
acsStopContainer pyContainer2 2>&1

acsStartContainer -py pyContainer1 > "$logs_path/pyContainer1.log" 2>&1 &
acsStartContainer -py pyContainer2 > "$logs_path/pyContainer2.log" 2>&1 &