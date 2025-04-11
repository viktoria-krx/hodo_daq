
#ifndef LOGGER_HH
#define LOGGER_HH

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <iostream>

class Logger {
public:
    static void init();
    static std::shared_ptr<spdlog::logger>& getLogger();

private:
    static std::shared_ptr<spdlog::logger> logger;
};

#endif