#include <rtadp/WorkerProcess.h>
#include <stdexcept>
#include <unistd.h> // for sleep

WorkerProcess::WorkerProcess(int worker_id, std::shared_ptr<WorkerManager> manager, const std::string& name, std::shared_ptr<WorkerBase> worker)
    : worker_id(worker_id), manager(manager), supervisor(manager->getSupervisor()), worker(worker), name(name),
      workersname(supervisor->getName() + "-" + manager->getName() + "-" + name),
      fullname(workersname + "-" + std::to_string(worker_id)),
      globalname("WorkerProcess-" + fullname), stop_event(false) {

    pidprocess = getpid();
    logger = supervisor->getLogger();
    worker->init(manager.get(), supervisor.get(), workersname, fullname);

    low_priority_queue = manager->getLowPriorityQueue();
    high_priority_queue = manager->getHighPriorityQueue();
    monitoringpoint = manager->getMonitoringPoint();

    start_time = std::chrono::system_clock::now().time_since_epoch().count();
    next_time = start_time;
    processed_data_count = 0;
    total_processed_data_count = 0;
    processing_rate = 0;

    manager->setWorkerStatus(worker_id, 0); // initialized

    start_timer(1);
    std::cout << globalname << " started " << pidprocess << std::endl;
    logger->info("WorkerProcess started", globalname);
}

WorkerProcess::~WorkerProcess() {
    stop();
}

void WorkerProcess::stop() {
    stop_event = true;

    low_priority_queue->notify_all();
    high_priority_queue->notify_all();

    if (timer_thread && timer_thread->joinable()) {
        timer_thread->join();
    }

    sleep(0.1);
}

void WorkerProcess::config(const nlohmann::json& configuration) {
    worker->config(configuration);
}

void WorkerProcess::run() {
    start_timer(1);

    try {
        while (!stop_event) {
            usleep(100); // equivalent to time.sleep(0.0001)

            if (manager->getProcessDataSharedValue() == 1) {
                manager->setWorkerStatus(worker_id, 2); // assume queue empty

                try {
                    auto high_priority_data = high_priority_queue->get();
                    process_data(high_priority_data, 1);
                } 
                catch (const std::out_of_range&) {
                    try {
                        auto low_priority_data = low_priority_queue->get();
                        process_data(low_priority_data, 0);
                    } 
                    catch (const std::out_of_range&) {
                        manager->setWorkerStatus(worker_id, 2); // waiting for new data
                    }
                }
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
    }

    stop();
    manager->setWorkerStatus(worker_id, 16); // stop
    std::cout << "WorkerProcess stop " << globalname << std::endl;
    logger->info("WorkerProcess stop", globalname);
}

void WorkerProcess::start_timer(int interval) {
    timer_thread = std::make_unique<std::thread>([this, interval]() {
        while (!stop_event) {
            std::this_thread::sleep_for(std::chrono::seconds(interval));
            workerop();
        }
    });
}

void WorkerProcess::workerop() {
    double elapsed_time = std::chrono::system_clock::now().time_since_epoch().count() - next_time;
    next_time = std::chrono::system_clock::now().time_since_epoch().count();
    processing_rate = processed_data_count / elapsed_time;
    manager->setProcessingRate(worker_id, processing_rate);
    total_processed_data_count += processed_data_count;
    manager->setTotalProcessedDataCount(worker_id, total_processed_data_count);

    std::cout << globalname << " Rate Hz " << processing_rate << " Current events " << processed_data_count
              << " Total events " << total_processed_data_count << std::endl;
    logger->info("Rate Hz " + std::to_string(processing_rate) + " Current events " +
                  std::to_string(processed_data_count) + " Total events " + std::to_string(total_processed_data_count),
                  globalname);
    processed_data_count = 0;
}

void WorkerProcess::process_data(const nlohmann::json& data, int priority) {
    manager->setWorkerStatus(worker_id, 8); // processing new data
    processed_data_count += 1;

    nlohmann::json dataresult;
    try {
        dataresult = worker->processData(data, priority);
    } 
    catch (const std::exception& e) {
        logger->critical(e.what(), globalname);
    }

    if (priority == 0) {
        manager->getResultLpQueue()->push(dataresult);
    } else {
        manager->getResultHpQueue()->push(dataresult);
    }
}
