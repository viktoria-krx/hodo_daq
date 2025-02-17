#include "logger.hh"

std::shared_ptr<spdlog::logger> Logger::logger;

void Logger::init() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

        auto max_size = 1048576 * 5; // 5 MB
        auto max_files = 10;
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/logfile.txt", max_size, max_files, true);
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
    // auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // console_sink->set_level(spdlog::level::info);
    // console_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    // // Create a file rotating logger with 5 MB size max and 3 rotated files
    // auto max_size = 1048576 * 5;
    // auto max_files = 3;
    // auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/logfile.txt", max_size, max_files, true);
    // file_sink->set_level(spdlog::level::info);
    // file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    // spdlog::logger logger("multi_sink", {console_sink, file_sink});
    // logger.set_level(spdlog::level::debug);


}

std::shared_ptr<spdlog::logger>& Logger::getLogger() {
    return logger;
}


