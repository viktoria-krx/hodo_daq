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
            // log->debug("{0} = {1}", key, value);
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

std::string getLockFilename(int runNumber) {
    std::map<std::string, std::string> config = loadConfig();

    std::ostringstream filename;
    filename << config["daq_path"] 
             <<"tmp/hodo_run_" << runNumber
             << ".lock";
    return filename.str(); 
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

    log->info("Processing binary data ...");

    std::ifstream file(binFile, std::ios::binary);
    Block block;
    long last_pos = 0;
    while (reader.readNextBlock(block, last_pos)) {
        for (auto& bank : block.banks) {
            for (auto& event : bank.events) {
                decoder.processEvent(bank.bankName, event);
            }   
        }
        last_pos = reader.currentPos;
    }

    log->info("Saving {}", binFile);
    decoder.writeTree();
    decoder.flush();

    log->info("Sorting ROOT file, merging TDC Data ...");
    DataFilter filter;
    filter.fileSorter(getRootFilename(runNumber).c_str(), 0, getDataFilename(runNumber).c_str());
    log->info("Filtering ROOT file, saving as EventTree ...");
    filter.filterAndSave(getDataFilename(runNumber).c_str(), 0);

}


long processNewData(FileReader& reader, DataDecoder& decoder, long startPos, uint32_t& lastEvent) {
    Block block;
    long lastPos = startPos;
    while (reader.readNextBlock(block, lastPos)) {
        for (auto& bank : block.banks) {
            for (auto& event : bank.events) {
                lastEvent = decoder.processEvent(bank.bankName, event);
            }
        }
        lastPos = reader.currentPos;
    }
    return lastPos;
}


void runLiveAnalysis(int runNumber) {
    auto log = Logger::getLogger();
    std::string binFile = getBinFilename(runNumber);
    std::string lockfile = getLockFilename(runNumber);

    log->info("Starting online analysis of {}", binFile);

    while (std::filesystem::exists(lockfile) & !(std::filesystem::exists(binFile))){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    FileReader reader(binFile);
    if (!reader.isOpen()) {
        log->error("Could not open file {}", binFile);
        return;
    }

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PUB);
    socket.bind("tcp://*:5555");

    DataDecoder decoder(getRootFilename(runNumber));

    const int pollingInterval = 1000; // ms
    long last_pos = 0;
    uint32_t last_event = 0;
    uint32_t this_event = 0;
    size_t last_size = 0;

    log->debug("Lockfile: {}", lockfile);
    log->info("Processing binary data ...");
    
    while (std::filesystem::exists(lockfile)) {
        std::ifstream file(binFile, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            log->warn("File closed or unavailable, retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(pollingInterval));
            continue;
        }
    
        size_t this_size = file.tellg();
        file.close();

        if (this_size > last_size && this_size%4 == 0) {
            log->info("New data detected, processing...");

            last_pos = processNewData(reader, decoder, last_pos, this_event);
            log->info("Saving {}", binFile);
            decoder.writeTree();
            decoder.flush();
            last_size = this_size;

            log->info("Sorting ROOT file, merging TDC Data ...");
            DataFilter filter;
            filter.fileSorter(getRootFilename(runNumber).c_str(), last_event, getDataFilename(runNumber).c_str());
            log->info("Filtering ROOT file, sending to GUI ...");
            filter.filterAndSend(getDataFilename(runNumber).c_str(), last_event, socket);

            
            last_event = this_event;

        }

        
        std::this_thread::sleep_for(std::chrono::milliseconds(pollingInterval));

    }

    try {
        log->info("Filtering ROOT file, saving as EventTree ...");
        DataFilter filter;
        filter.filterAndSave(getDataFilename(runNumber).c_str(), 0);
    } catch (...) {
        log->error("File {} could not be saved.", getDataFilename(runNumber).c_str());
    }


    log->debug("Lockfile does not exist, run {:d} is not ongoing.", runNumber);

}



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