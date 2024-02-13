# Copyright (C) 2024 INAF
# This software was provided as IKC to the Cherenkov Telescope Array Observatory
# This software is distributed under the terms of the BSD-3-Clause license
#
# Authors:
#
#    Andrea Bulgarelli <andrea.bulgarelli@inaf.it>
#
import zmq
import json
import time
def subscribe_data(socketstring):
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect(socketstring)  # Subscriber connects to the same address

    # Subscribe to all topics (empty string)
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    time.sleep(0.1)

    try:
        while True:
            message = socket.recv_string()
            data = json.loads(message)
            print(f"Received: {data}")

    except KeyboardInterrupt:
        print("Subscriber stopped.")



if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python ProcessMonitoring.py <socket>")
        sys.exit(1)

    socket = sys.argv[1]

    subscribe_data(socket)

