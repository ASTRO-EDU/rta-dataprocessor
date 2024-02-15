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
import sys



if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python ProcessMonitoring.py <socket>")
        sys.exit(1)

    socketstring = sys.argv[1]

    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.bind(socketstring)

    while True:
        message = socket.recv_string()
        #decoded_message = json.loads(message)
        decoded_message = message
        # Esegui l'aggregazione o altre operazioni sui dati ricevuti
        print("Dati ricevuti:", decoded_message)

