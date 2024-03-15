# rta-dataprocessor

## rtadp-proto
- env: Dockerfile
- rtadp-proto: a prototype with 2 data processors
- rtadp-proto/testavro: the producers

cd rtadp-proto/testavro
python AVROProducer.py ../config.json lp 0.5 RTADP1

cd workers
python ProcessMonitoring.py ../config.json

cd rtadp-proto/
python ProcessDataConsumer1.py ../config.json
python ProcessDataConsumer2.py ../config.json  

cd workers
python Command.py ../config.json start all
python Command.py ../config.json shutdown all

# ACS Steps
docker login to gitlab inaf and:
```
docker pull git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:latest
```
bootstrap container using the utility bootstrap.sh
```
./bootstrap.sh latest <dev_name>
```
it will produce an image `git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:latest_dev_name`

start docker image mounting the root of the git repository in `/home/astrisw/src`
```
 docker run -v "$PWD:/home/astrisw/src" -v "$PWD/.bashrc_rtadataprocessor:/home/astrisw/.bashrc" --entrypoint "/home/astrisw/src/env/entrypoint.sh" -d git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:latest_<dev_name>
```
it will give the `cont_id` back
enter container 
```
docker exec -it <cont_id> bash -l
```
shell will become like:
```
<cont_id> astrisw:~ >
```
Install python deps:
```
python -m pip install --user -r /home/astrisw/src/env/venv/requirements_minimal_acs
```


# Steps inside the container:
    
To install the components code: 
check for acs configuration database errors
```
cdbChecker
```
if no errors:
build and install IDL
```
cd $RTA_DP_ROOT/rtadp-proto/ACS/idl/
make clean all install 
```
build and install components implementation
```
cd $RTA_DP_ROOT/rtadp-proto/ACS/src/
make clean all install 
```

to start ACS in foreground: (execute it from the defined "acs manager" docker)
```
acsStart
``` 
or to restart acs safely, with logs in `$ACSDATA/logs/`
```
cd $RTA_DP_ROOT/rtadp-proto/ACS/; ./start_acs.sh
cd $RTA_DP_ROOT/rtadp-proto/ACS/; ./start_acs_container.sh <local|distributed>
``` 

to stop ACS 
```
acsStop
```

to start/restart containers: (logs in `$ACSDATA/logs`)
local: containers are started in the localhost
distributed: check services/names inside the script where containers are started
```
cd $RTA_DP_ROOT/rtadp-proto/ACS/; ./start_acs_container.sh <local|distributed>
```

## To modify code and test it 
- build code
- restart containers

## python client 
```
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```

get component and execute a method
```
commander=client.getComponent("Commander")
import rtamanager
commander.sendCommand(rtamanager.START,"test")
```

get a dynamyc component and execute a method
```
dataprocessor = client.getDynamicComponent("DATAPROCESSOR_1", "IDL:scada/rtamanager/DataProcessor:1.0","rtamanagerImpl.DataProcessorImpl", "pyContainer1" )
dataprocessor.configure("test")
```

