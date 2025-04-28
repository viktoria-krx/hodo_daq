#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <sys/stat.h>  // For file size checking
#include <cstdlib>
#include <filesystem>
#include "logger.hh"
#include "fileReader.hh"
#include "dataDecoder.hh"
#include "dataFilter.hh"

#define CHECK_INTERVAL_MS 1000   // Check every 500ms

namespace fs = std::filesystem;

long get_file_size(const std::string& filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
        return st.st_size;
    return -1;
}

// Config file for run number: 
const std::string runconfig = "../../config/daq_config.conf";
std::map<std::string, std::string> config;


/**
 * @brief Load configuration from a file.
 *
 * This function reads a configuration file at the location specified by
 * the global variable `runconfig`. The file should contain lines of the form
 * "key=value". The resulting key-value pairs are stored in the global
 * `config` map.
 *
 * @return the config map
 */
std::map<std::string, std::string> loadConfig() {

    auto log = Logger::getLogger();

    std::ifstream file(runconfig);
    // std::map<std::string, std::string> config;
    std::string line;
    
    if (!file.is_open()) {
        log->error("Could not open config file!");
        return config;
    }

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            config[key] = value;
            log->debug("{0} = {1}", key, value);
        }
    }
    return config;
}

std::string getBinFilename(int runNumber) {
    std::map<std::string, std::string> config = loadConfig();

    std::ostringstream filename;
    filename << config["daq_path"] 
             << config["data_path"] << "/"
             << config["file_prefix"]
             << std::setw(6) << std::setfill('0') << runNumber
             << ".bin";

    return filename.str(); 
}

std::string getRootFilename(int runNumber) {
    std::map<std::string, std::string> config = loadConfig();

    std::ostringstream filename;
    filename << config["daq_path"] 
             << config["raw_path"] << "/"
             << config["raw_prefix"]
             << std::setw(6) << std::setfill('0') << runNumber
             << ".root";

    return filename.str(); 
}

std::string getDataFilename(int runNumber) {
    std::map<std::string, std::string> config = loadConfig();

    std::ostringstream filename;
    filename << config["daq_path"] 
             << config["ana_path"] << "/"
             << config["ana_prefix"]
             << std::setw(6) << std::setfill('0') << runNumber
             << ".root";

    return filename.str(); 
}


void monitor_file(int runNumber) {
    auto log = Logger::getLogger();
    
    zmq::context_t context(1);
    zmq::socket_t analysis_socket(context, ZMQ_PUB);
    analysis_socket.bind("tcp://*:5556");  // Publish analyzed data

    std::string binFile = getBinFilename(runNumber);

    FileReader reader(binFile);
    if (!reader.isOpen()) {
        log->error("Could not open file {0}", binFile);
        return; 
    }

    DataDecoder decoder(getRootFilename(runNumber));
    long last_size = reader.getFileSize();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));

        long current_size = reader.getFileSize();
        if (current_size > last_size) {
            std::ifstream file(binFile, std::ios::binary);
            Block block;
            if (reader.readNextBlock(block, last_size)) {
                for (auto& bank : block.banks) {
                    for (auto& event : bank.events) {
                        decoder.processEvent(bank.bankName, event.data);
                    }   
                }
            }

            last_size = current_size;  // Update last known file size
        }
    }
    decoder.writeTree();
    decoder.flush();
}

void runOfflineAnalysis(int runNumber) {
    auto log = Logger::getLogger();
    std::string binFile = getBinFilename(runNumber);

    FileReader reader(binFile);
    if (!reader.isOpen()) {
        log->error("Could not open file {0}", binFile);
        return;
    }

    DataDecoder decoder(getRootFilename(runNumber));

    std::ifstream file(binFile, std::ios::binary);
    Block block;
    long last_pos = 0;
    while (reader.readNextBlock(block, last_pos)) {
        for (auto& bank : block.banks) {
            for (auto& event : bank.events) {
                decoder.processEvent(bank.bankName, event.data);
            }   
        }
        last_pos = reader.currentPos;
    }

    decoder.writeTree();
    decoder.flush();

}

void runLiveAnalysis(int runNumber) {}

int main(int argc, char* argv[]) {

    Logger::init();
    auto log = Logger::getLogger();

    log->info("Hiya!");
    log->debug("Oy!");

    bool liveMode = false;


    if (argc < 2) {
        log->error("Usage: ./hodo_analysis <run_number>");
        return 1;
    }

    int argIndex = 1;

    if (std::string(argv[1]) == "-l") {
        liveMode = true;
        argIndex++;
    }

    if (argIndex >= argc) {
        log->error("Error: Missing filename.");
        return 1;
    }

    int runNumber = std::stoi(argv[argIndex]);

    // Info print
    log->info("Running in {} mode.", (liveMode ? "LIVE" : "OFFLINE"));
    log->info("Analyzing run: {:d}", runNumber);

    // Now you can call your main logic:
    if (liveMode) {
        runLiveAnalysis(runNumber);
    } else {
        runOfflineAnalysis(runNumber);
    }

    return 0;
}