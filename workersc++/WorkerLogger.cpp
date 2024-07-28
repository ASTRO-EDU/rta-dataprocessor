#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/fmt.h>
#include <string>

class WorkerLogger {
public:
    static const int SYSTEM_LEVEL_NUM = spdlog::level::level_enum::trace + 1; // Define a new custom log level

    WorkerLogger(const std::string& logger_name = "my_logger", const std::string& log_file = "my_log_file.log", spdlog::level::level_enum level = spdlog::level::debug) {
        // Create a logger
        logger = spdlog::basic_logger_mt(logger_name, log_file);
        logger->set_level(level);

        // Set a custom formatter
        logger->set_pattern("%Y-%m-%d %H:%M:%S.%e - %l - %v");

        // Add the new level SYSTEM
        spdlog::level::level_enum system_level = static_cast<spdlog::level::level_enum>(SYSTEM_LEVEL_NUM);
        spdlog::level::level_enum max_level = static_cast<spdlog::level::level_enum>(spdlog::level::n_levels);

        if (system_level > max_level) {
            spdlog::level::n_levels = system_level + 1;
        }
        spdlog::set_level(spdlog::level::trace);

        spdlog::level::level_string_views[system_level] = "SYSTEM";
        spdlog::details::level_to_string_views[system_level] = "SYSTEM";

        // Add the system method to the logger
        auto custom_log = [this](const std::string& msg, const std::string& extra) {
            this->log_system(msg, extra);
        };
        logger->trace = custom_log;
    }

    void debug(const std::string& msg, const std::string& extra = "") {
        logger->debug("{} - \"{}\"", extra, msg);
    }

    void info(const std::string& msg, const std::string& extra = "") {
        logger->info("{} - \"{}\"", extra, msg);
    }

    void warning(const std::string& msg, const std::string& extra = "") {
        logger->warn("{} - \"{}\"", extra, msg);
    }

    void error(const std::string& msg, const std::string& extra = "") {
        logger->error("{} - \"{}\"", extra, msg);
    }

    void critical(const std::string& msg, const std::string& extra = "") {
        logger->critical("{} - \"{}\"", extra, msg);
    }

    void system(const std::string& msg, const std::string& extra = "") {
        logger->log(static_cast<spdlog::level::level_enum>(SYSTEM_LEVEL_NUM), "{} - \"{}\"", extra, msg);
    }

private:
    std::shared_ptr<spdlog::logger> logger;

    void log_system(const std::string& msg, const std::string& extra) {
        if (logger->should_log(static_cast<spdlog::level::level_enum>(SYSTEM_LEVEL_NUM))) {
            logger->log(static_cast<spdlog::level::level_enum>(SYSTEM_LEVEL_NUM), "{} - \"{}\"", extra, msg);
        }
    }
};

