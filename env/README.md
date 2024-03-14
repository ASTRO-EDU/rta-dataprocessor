# gammaflash-env
gammaflash-env

## Install and run the environment

1. Install python3 (current supported version 3.8.8)

2. Create the environment

```
python3 -m venv /path/to/new/virtual/environment
```

3. Install requirements

```
pip install -r venv/requirements.txt
```

4. Run the activate script

```
source /path/to/new/virtual/environment/bin/activate
```

=============
Docker image

docker system prune

-----

On MAC platform
docker build --platform linux/amd64 -t worker:1.0.0 -f ./Dockerfile.amd .
docker build --platform linux/arm64 -t worker:1.0.0 -f ./Dockerfile.arm .


On Linux platform
docker build -t worker:1.0.0 -f ./Dockerfile.amd .

-----

docker build -t worker:1.0.0 .

./bootstrap.sh worker:1.0.0 agileobs

docker run -it -d -v /home/agileobs/worker/workspace:/home/worker/workspace -v /data02/:/data02/  -p 8001:8001 --name rtadp1 worker:1.0.0_agileobs /bin/bash

docker exec -it rtadp1 /bin/bash
cd
. entrypoint.sh

nohup jupyter-lab --ip="*" --port 8001 --no-browser --autoreload --NotebookApp.token='worker2024#'  --notebook-dir=/home/worker/workspace --allow-root > jupyterlab_start.log 2>&1 &

SETUP ENV DEV
------------

docker run -it -d -v /Users/bulgarelli/devel/astri/rta-dataprocessor/:/home/worker/workspace  --name rtadp1 worker:1.0.0 /bin/bash

docker run -it -d -v /Users/bulgarelli/devel/astri/rta-dataprocessor/:/home/worker/workspace  --name rtadp2 worker:1.0.0 /bin/bash

docker run -it -d -v /Users/bulgarelli/devel/astri/rta-dataprocessor/:/home/worker/workspace  --name rtadp3 worker:1.0.0 /bin/bash

docker exec -it rtadp1 bash
docker exec -it rtadp2 bash
docker exec -it rtadp3 bash

