# Kafka consumer

## startup the docker compose

cd testkafka
docker compose up -d 

## install kafka python package
- The first time you enter in the worker container 
- it is required the installation of the confluent kafka package 

docker compose exec worker bash
source activate worker
pip install confluent_kafka

## test the pipeline
- activate the environment in each shell

source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"

### shell 1
docker compose exec worker bash
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
cd testkafka
python producer.py 

### shell 2
docker compose exec worker bash
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
cd testkafka
python ProcessDataConsumer1.py ../config_kafka.json

### shell 3
docker compose exec worker bash
cd workers
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"
python ProcessMonitoring.py ../config_kafka.json

### shell 4
docker compose exec worker bash
cd workers
source activate worker
export PYTHONPATH="/home/worker/workspace/workers/"

- start the pipeline

python Command.py ../config_kafka.json start all

- stop the pipeline

python Command.py ../config_kafka.json shutdown all

## stop the containers
exit
docker compose down -v