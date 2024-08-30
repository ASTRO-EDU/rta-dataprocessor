# rta-dataprocessor

## rtadp-proto
- env: Dockerfile
- rtadp-proto: a prototype with 2 data processors
- testavro: the producers

cd testavro
python AVROProducer.py ../config.json lp 0.5 RTADP1

### monitoring forwarder
cd workers
python Forwarder.py ../config_forwarder.json
python ProcessMonitoring.py ../config_forwarder.json

cd rtadp-proto/
python ProcessDataConsumer1.py ../config_forwarder.json
python ProcessDataConsumer2.py ../config_forwarder.json  

### direct monitoring
cd workers
python ProcessMonitoring.py ../config.json

cd rtadp-proto/
python ProcessDataConsumer1.py ../config.json
python ProcessDataConsumer2.py ../config.json  

cd workers
python SendCommand.py ../config.json start all
python SendCommand.py ../config.json shutdown all