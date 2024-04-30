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

## local deployment
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


### Steps inside the container:
    
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
cd $RTA_DP_ROOT/rtadp-proto/ACS/; ./start_acs_containers.sh <local|distributed>
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

## distributed deployment

Start containers using docker compose
```
cd $RTA_DP_ROOT/rtadp-proto/ACS/;
export COMPOSE_PROJECT_NAME=user_$USER
docker-compose up -d
docker-compose ps
```

Enter inside the acsmanager container
```
docker-compose exec acsmanager bash -l
```

Go to procedure "Steps inside the container"

Stop contaienrs
```
docker-compose down
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

# ACS rtaproto 

## Local test
1. __RTADP1, RTADP2 and Commander__ 
```
cd
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```
```
import os
import time
import rtamanager

cwd = os.getcwd()
json_path = os.path.join(cwd, 'src', 'config.json')

dataprocessor1 = client.getDynamicComponent("DATAPROCESSOR_1", "IDL:scada/rtamanager/DataProcessor:1.0","rtamanagerImpl.DataProcessorImpl", "pyContainer1" )
dataprocessor1.configure(json_path, 'RTADP1')
dataprocessor1.start()
print("started dataprocessor1")

dataprocessor2 = client.getDynamicComponent("DATAPROCESSOR_2", "IDL:scada/rtamanager/DataProcessor:1.0","rtamanagerImpl.DataProcessorImpl", "pyContainer2" )
dataprocessor2.configure(json_path, 'RTADP2')
dataprocessor2.start()
print("started dataprocessor2")

time.sleep(3)

commander=client.getComponent("Commander")
json_path = os.path.join(cwd, 'src', 'config.json')
commander.configure(json_path, 'CommandCenter')
commander.sendCommand(rtamanager.START,'all')
print("Start command sent")

time.sleep(3)

commander.sendCommand(rtamanager.STOP,'all')
print("Stop command sent")

time.sleep(2)

commander.sendCommand(rtamanager.CLEANEDSHUTDOWN,'all')
print("Cleanedshutdown command sent")
time.sleep(5)

exit
```

3. __Monitoring__
```
cd src/worker
python ProcessMonitoring.py ../config.json 
```
otherwise in ACS container
```
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```
```
monitoring=client.getComponent("Monitoring")
import os
cwd = os.getcwd()
json_path = os.path.join(cwd, 'src', 'config.json')
monitoring.configure(json_path, 'CommandCenter')
monitoring.start()
```

5. __Producer__
```
cd src/rtadp-proto/testavro/
python AVROProducer.py ../../config.json lp 3 RTADP1
```

## Distributed test
1. __RTADP1__
```
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```
```
dataprocessor = client.getDynamicComponent("DATAPROCESSOR_1", "IDL:scada/rtamanager/DataProcessor:1.0","rtamanagerImpl.DataProcessorImpl", "pyContainer1" )
import os
cwd = os.getcwd()
json_path = os.path.join(cwd, 'src', 'config_distributed.json')
dataprocessor.configure(json_path, 'RTADP1')
dataprocessor.start()
```

2. __RTADP2__
```
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```
```
dataprocessor = client.getDynamicComponent("DATAPROCESSOR_2", "IDL:scada/rtamanager/DataProcessor:1.0","rtamanagerImpl.DataProcessorImpl", "pyContainer1" )
import os
cwd = os.getcwd()
json_path = os.path.join(cwd, 'src', 'config_distributed.json')
dataprocessor.configure(json_path, 'RTADP2')
dataprocessor.start()
```

3. __COMMANDER__
```
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```
```
commander=client.getComponent("Commander")
import rtamanager
import os
cwd = os.getcwd()
json_path = os.path.join(cwd, 'src', 'config_distributed.json')
commander.configure(json_path, 'CommandCenter')
commander.sendCommand(rtamanager.START,'all')
```

4. __Monitoring__
```
cd src/worker
python ProcessMonitoring.py ../config_distributed.json 
```
otherwise in ACS container
```
ipython -i ~/src/rtadp-proto/ACS/python_client.py
```
```
monitoring=client.getComponent("Monitoring")
import os
cwd = os.getcwd()
json_path = os.path.join(cwd, 'src', 'config_distributed.json')
monitoring.configure(json_path, 'CommandCenter')
monitoring.start()
```

5. __Producer__
```
cd src/rtadp-proto/testavro/
python AVROProducer.py ../../config_distributed.json lp 3 RTADP1
```