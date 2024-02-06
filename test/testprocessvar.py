import multiprocessing
import time

class MyProcess(multiprocessing.Process):
    def __init__(self, process_id):
        super(MyProcess, self).__init__()
        self.process_id = process_id
        self.internal_variable = 0

    def run(self):
        for i in range(5):
            print(f"Process {self.process_id} - Internal Variable: {self.internal_variable}")
            time.sleep(1)

    def set_internal_variable(self, value):
        self.internal_variable = value

def main():
    processes = []
    for i in range(3):
        process = MyProcess(i)
        processes.append(process)
        process.start()

    # Modifica la variabile interna direttamente dal processo genitore
    for process in processes:
        new_value = int(input(f"Inserisci un nuovo valore per il processo {process.process_id}: "))
        process.internal_variable = new_value
        #process.set_internal_variable(new_value)

    for process in processes:
        print(process.internal_variable)

    for process in processes:
        process.join()

if __name__ == "__main__":
    main()

