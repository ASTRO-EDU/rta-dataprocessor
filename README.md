# rta-dataprocessor

## rtadp-proto
- env: Dockerfile
- rtadp-proto: a prototype with 2 data processors
- testavro: the producers

cd testavro
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
python -m pip install --user /home/astrisw/src/env/venv/requirements_minimal_acs
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
cd $RTA_DP_ROOT/rta_dataprocessor/idl/
make clean all install 
```
build and install components implementation
```
cd $RTA_DP_ROOT/rta_dataprocessor/src/
make clean all install 
```

to start ACS in foreground:
```
acsStart
``` 
or to restart acs safely, with logs in `$RTA_DP_ROOT/logs`
```
cd $RTA_DP_ROOT; ./start_acs.sh
``` 

to stop ACS 
```
acsStop
```

to start/restart containers: (logs in `$RTA_DP_ROOT/logs`)
```
./start_acs_container.sh
```

## To modify code and test it 
- build code
- restart containers



