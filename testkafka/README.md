# Kafka consumer

## startup the docker compose
```bash
cd testkafka
docker compose up -d 
```
## install kafka python package
- The first time you enter in the worker container 
- it is required the installation of the confluent kafka package 
```bash
docker compose exec worker bash
source activate worker
pip install confluent_kafka
```
## test the pipeline
- activate the environment in each shell
```bash
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
```
### shell 1
```bash
docker compose exec worker bash
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
cd testkafka
python producer.py 
```
### shell 2
```bash
docker compose exec worker bash
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
cd testkafka
python ProcessDataConsumer1.py ../config_kafka.json
```
### shell 3
```bash
docker compose exec worker bash
cd workers
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
python ProcessMonitoring.py ../config_kafka.json
```
### shell 4
```bash
docker compose exec worker bash
cd workers
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
```
- start the pipeline
```bash
python Command.py ../config_kafka.json start all
```
- stop the pipeline
```bash
python Command.py ../config_kafka.json shutdown all
```
## stop the containers
```bash
exit
docker compose down -v
```