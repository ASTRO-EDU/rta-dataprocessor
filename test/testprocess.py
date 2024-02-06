import multiprocessing
import time

def process_function(process_id):
    while True:
#        print(f"Process-{process_id} is running")
#        time.sleep(1)
        pass

if __name__ == "__main__":
    processes = []

    # Creazione e avvio di 5 processi
    for i in range(5):
        process = multiprocessing.Process(target=process_function, args=(i,))
        processes.append(process)
        process.start()

    try:
        # Esegui il programma principale con uno sleep infinito
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        # Se viene ricevuta un'interruzione da tastiera, interrompi tutti i processi
        print("Ricevuta interruzione da tastiera. Terminazione in corso.")
        for process in processes:
            process.terminate()
            process.join()

    print("Programma terminato.")

