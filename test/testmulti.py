import threading
import time

def thread_function(thread_id):
    while True:
        #print(f"Thread-{thread_id} is running")
        #time.sleep(1)
        #print("ciao")
        pass

if __name__ == "__main__":
    threads = []

    # Creazione e avvio di 5 thread
    for i in range(5):
        thread = threading.Thread(target=thread_function, args=(i,), daemon=True)
        threads.append(thread)
        thread.start()

    try:
        # Esegui il programma principale con uno sleep infinito
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        # Se viene ricevuta un'interruzione da tastiera, interrompi tutti i thread
        print("Ricevuta interruzione da tastiera. Terminazione in corso.")
        for thread in threads:
            thread.join()

    print("Programma terminato.")

