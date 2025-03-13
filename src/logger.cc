#include "logger.hh"

std::shared_ptr<spdlog::logger> Logger::logger;

/**
 * @brief Initializes the logger by setting up two sinks: one for the console and one for a rotating log file.
 * 
 * The console sink is set to debug level and the file sink is set to info level.
 * The file sink is set to rotate every 5 MB and keep up to 10 files.
 * The pattern for both sinks is set to include the date, time, log level and log message.
 * 
 * If the logger initialization fails, an error message is printed to the console.
 */
void Logger::init() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        auto max_size = 1048576 * 5; // 5 MB
        auto max_files = 100;
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("../logs/logfile.txt", max_size, max_files, true);
        file_sink->set_level(spdlog::level::info);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

        // Assign to the static member
        logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, file_sink});
        
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info); 
        spdlog::register_logger(logger);        

    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }

}

std::shared_ptr<spdlog::logger>& Logger::getLogger() {
    return logger;
}


