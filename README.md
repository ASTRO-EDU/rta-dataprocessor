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