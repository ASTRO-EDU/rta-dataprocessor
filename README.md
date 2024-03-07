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
docker pull git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:latest

bootstrap container using the utility bootstrap.sh
./bootstrap.sh latest <dev_name>
it will produce an image git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:latest_dev_name

start docker image mounting the root of the git repository in /home/astrisw/src
docker run -v "$PWD:/home/astrisw/src" -dt git.ia2.inaf.it:5050/astri/scada/scada/acs_aug2023:latest_dev_name bash -l
it will give the cont_id back
enter container 
docker exec -it <cont_id> bash -l


Steps inside the container:
    create a build directory
    cd src
    mkdir build

    ACS Settings:
    export ACSDATA=~/src/ACSDATA
    export INTROOT=~/src/INTROOT
    export ACS_CDB=~/src/rta_dataprocessor
    export ACS_RETAIN=1
    source /alma/ACS/ACSSW/config/.acs/.bash_profile.acs 
    getTemplateForDirectory INTROOT $INTROOT
    getTemplateForDirectory ACSDATA $ACSDATA

    build IDL and install the code:
    cd ~/src/rta_dataprocessor/src
    cdbChecker
    make clean all install 

    to start ACS:
    acsStart

    to stop ACS 
    acsStop

    TBD....


