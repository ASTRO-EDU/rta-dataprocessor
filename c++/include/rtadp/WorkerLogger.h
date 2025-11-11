#ifndef WORKERLOGGER_H
#define WORKERLOGGER_H
 
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/fmt.h>
#include <string>
#include <memory>
 
class WorkerLogger {
public:
    // Define a new custom log level
    static const int SYSTEM_LEVEL_NUM = spdlog::level::level_enum::trace + 1;

    // Constructor to initialize the WorkerLogger with a logger name, log file, and log level
    WorkerLogger(const std::string& logger_name = "my_logger", const std::string& log_file = "my_log_file.log", spdlog::level::level_enum level = spdlog::level::trace, const std::string& logging_mode = "both");
 
    
    // Metodi di log dichiarati condizionalmente tramite macro
#ifdef ENABLE_LOGGING
    void debug(const std::string& msg, const std::string& extra = "");
    void info(const std::string& msg, const std::string& extra = "");
    void warning(const std::string& msg, const std::string& extra = "");
    void error(const std::string& msg, const std::string& extra = "");
    void critical(const std::string& msg, const std::string& extra = "");
    void trace(const std::string& msg, const std::string& extra = "");
#else
    void debug(const std::string& msg, const std::string& extra = "") {}
    void info(const std::string& msg, const std::string& extra = "") {}
    void warning(const std::string& msg, const std::string& extra = "") {}
    void error(const std::string& msg, const std::string& extra = "") {}
    void critical(const std::string& msg, const std::string& extra = "") {}
    void trace(const std::string& msg, const std::string& extra = "") {}
#endif

 
private:
    std::shared_ptr<spdlog::logger> logger;  // Shared pointer to the spdlog logger
 
    // Helper method to log system-level messages
    void log_system(const std::string& msg, const std::string& extra = "");
};
 
#endif // WORKERLOGGER_H