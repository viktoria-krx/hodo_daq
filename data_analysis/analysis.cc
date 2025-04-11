#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include <sys/stat.h>  // For file size checking
#include "logger.hh"
#include "fileReader.hh"
#include "dataDecoder.hh"
#include "dataFilter.hh"

#define BINARY_FILE "data.bin"  // Change this to match your DAQ file
#define CHECK_INTERVAL_MS 500   // Check every 500ms

long get_file_size(const std::string& filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
        return st.st_size;
    return -1;
}

void analyze_data(std::vector<uint32_t>& data, zmq::socket_t& socket) {
    // Example: Compute an average value from the new data
    if (data.empty()) return;

    float sum = 0;
    for (uint32_t value : data) {
        sum += value;
    }
    float avg = sum / data.size();

    // Send analysis results over ZeroMQ
    zmq::message_t result_msg(50);
    snprintf((char*)result_msg.data(), 50, "%ld %f", time(nullptr), avg);
    socket.send(result_msg, zmq::send_flags::none);
}

void monitor_file() {
    auto log = Logger::getLogger();
    
    zmq::context_t context(1);
    zmq::socket_t analysis_socket(context, ZMQ_PUB);
    analysis_socket.bind("tcp://*:5556");  // Publish analyzed data

    FileReader reader(BINARY_FILE);
    if (!reader.isOpen()) {
        log->error("Could not open file {0}", BINARY_FILE);
        return; 
    }

    DataDecoder decoder("output.root");
    long last_size = reader.getFileSize();

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_INTERVAL_MS));

        long current_size = reader.getFileSize();
        if (current_size > last_size) {
            std::ifstream file(BINARY_FILE, std::ios::binary);
            Block block;
            if (reader.readNextBlock(block, last_size)) {
                for (auto& bank : block.banks) {
                    for (auto& event : bank.events) {
                        decoder.processEvent(bank.bankName, event.data);
                    }   
                }
            }
            // if (!file) {
            //     std::cerr << "Error: Could not open file.\n";
            //     continue;
            // }

            // file.seekg(last_size);  // Move to last read position
            // std::vector<uint32_t> new_data((current_size - last_size) / sizeof(uint32_t));
            // file.read(reinterpret_cast<char*>(new_data.data()), new_data.size() * sizeof(uint32_t));
            // file.close();

            // Process new data
            // analyze_data(new_data, analysis_socket);

            last_size = current_size;  // Update last known file size
        }
    }
    decoder.writeTree();
}

int main() {

    Logger::init();
    auto log = Logger::getLogger();

    log->info("Hiya!");
    log->debug("Oy!");

    monitor_file();
    return 0;
}