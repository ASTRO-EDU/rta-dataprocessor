#include <rtadp/WorkerLogger.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/null_sink.h"

WorkerLogger::WorkerLogger(const std::string& logger_name, const std::string& log_file, spdlog::level::level_enum level, const std::string& logging_mode) {
    std::vector<spdlog::sink_ptr> sinks;

    if (logging_mode == "file" || logging_mode == "both") {
        // Sink per il file
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        sinks.push_back(file_sink);
    }

    if (logging_mode == "console" || logging_mode == "both") {
        // Sink per la console
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l]%$ %v");
        sinks.push_back(console_sink);
    }

    if (!sinks.empty()) {
        // Crea un logger solo se ci sono sink configurati
        logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());
        logger->set_level(level);
    }
    else {
        // Usa un sink nullo
        auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
        logger = std::make_shared<spdlog::logger>(logger_name, null_sink);
    }
}

#ifdef ENABLE_LOGGING
void WorkerLogger::debug(const std::string& msg, const std::string& extra) {
    logger->debug("{} - \"{}\"", extra, msg);
}

void WorkerLogger::info(const std::string& msg, const std::string& extra) {
    logger->info("{} - \"{}\"", extra, msg);
}

void WorkerLogger::warning(const std::string& msg, const std::string& extra) {
    logger->warn("{} - \"{}\"", extra, msg);
}

void WorkerLogger::error(const std::string& msg, const std::string& extra) {
    logger->error("{} - \"{}\"", extra, msg);
}

void WorkerLogger::critical(const std::string& msg, const std::string& extra) {
    logger->critical("{} - \"{}\"", extra, msg);
}

void WorkerLogger::trace(const std::string& msg, const std::string& extra) {
    logger->trace("{} - \"{}\"", extra, msg);
}
#endif
