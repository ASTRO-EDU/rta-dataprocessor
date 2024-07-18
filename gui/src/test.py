import zmq
context = zmq.Context()
socket = context.socket(zmq.PULL)
socket.connect("tcp://127.0.0.1:25562")  # Esempio: connessione TCP su localhost, porta 5555

while True:
   msg = socket.recv_string()
   print(msg)
