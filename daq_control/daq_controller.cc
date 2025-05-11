#include "v1190.hh"
#include "v2495.hh"
#include "dataBanks.hh"
#include "vmeInterface.hh"
#include "logger.hh"
#include "tcp_server.hh"

#include <iostream>
#include <vector>
#include <memory>
#include <stdlib.h>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

#define NUM_TDCS 4
#define NUM_FPGAS 1

v1190 *tdcs[NUM_TDCS];
bool V1190Status[NUM_TDCS];
v2495 *fpgas[NUM_FPGAS];
int V2495Status[NUM_FPGAS];

// Define all base addresses:
// static const unsigned int vmeBaseAddress = 0x32100000;
char* vmeIPAddress = "192.168.1.254\0";
static const unsigned int TDCbaseAddresses[] = {0x90900000, 0x90910000, 0x90920000, 0x90930000};
const char* FPGAbaseAddress = "30300000";
// static const unsigned int FPGAserialNumber = 28;

static const int vmeConnType = cvETH_V4718;
int handle = -1;
VMEInterface vme(vmeConnType, vmeIPAddress);

// Config file for run number: 
const std::string runconfig = "../../config/daq_config.conf";
std::map<std::string, std::string> config;

// Thread safe readout 
std::queue<DataBank> bankQueue;
std::mutex bankQueueMutex;
std::condition_variable dataAvailable;
bool stopReadout = false;
std::vector<std::thread> readoutThreads;
std::vector<bool> tdcReading(NUM_TDCS, false);
std::thread pollingThread;
std::thread processingThread;

// Thread safe writing
std::queue<std::vector<uint32_t>> blockQueue;
std::mutex blockQueueMutex;
std::condition_variable blockQueueCond;
bool stopWriter = false;
std::thread fileWriter;

uint32_t blockID = 0; // ID of each BLT block

bool all_init = false;
bool is_running = false;

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

            if (!is_running){
                log->debug("{0} = {1}", key, value);
            }
            
        }
    }
    return config;
}

/**
 * @brief Save the current configuration to a file.
 *
 * The configuration is written to the file specified by the global
 * variable `runconfig`. The file should contain lines of the form
 * "key=value".
 *
 * @param config The configuration to be saved.
 */
void saveConfig(const std::map<std::string, std::string>& config) {
    std::ofstream file(runconfig);
    for (const auto& [key, value] : config) {
        file << key << "=" << value << "\n";
    }
}


/**
 * @brief Create a filename for a run given a run number, a path, and a
 *        filename prefix.
 *
 * The filename is created by concatenating the path and the prefix with the
 * run number (padded with zeros to at least 6 digits).
 *
 * @param runNumber The run number.
 * @param path The path to the file.
 * @param prefix The filename prefix.
 *
 * @return A pointer to a C-string containing the filename. The caller must
 *         free this memory when it is no longer needed.
 */
char* getRunFilename(int runNumber, const std::string& path, const std::string& prefix) {
    std::ostringstream filename;
    filename << path << "/" << prefix << std::setw(6) << std::setfill('0') << runNumber << ".bin";

    return strdup(filename.str().c_str());  // Allocate memory for C-string (must be freed)
}


/**
 * @brief Starts a readout thread for a given TDC.
 *
 * This function launches a new thread that continuously reads data from
 * the specified TDC (Time-to-Digital Converter) and stores the data in a
 * thread-safe queue. The thread runs until `stopReadout` is set to true.
 * Each `DataBank` is named with the provided bankName.
 *
 * @param tdc Reference to the TDC from which to read data.
 * @param bankName The name to be assigned to each `DataBank` created.
 */
void startReadoutThread(v1190& tdc, std::string bankName) {
    readoutThreads.emplace_back([&tdc, bankName] () {
        // while(!stopReadout) {
            DataBank dataBank(bankName.c_str());
            unsigned int wordsRead = tdc.BLTRead(dataBank);

            if (wordsRead > 0) {
                std::lock_guard<std::mutex> lock(bankQueueMutex);
                bankQueue.push(std::move(dataBank));
                dataAvailable.notify_one();
            }
            int tdcID = bankName.back() - '0';
            tdcReading[tdcID] = false;
            //std::this_thread::sleep_for(std::chrono::milliseconds(10));

        //}
    });
}

/**
 * @brief Write a block of data to a binary file.
 *
 * This function appends the given data block to a binary file associated
 * with the current run number. The filename is constructed using the
 * "run_number", "data_path" and "file_prefix" configuration values.
 *
 * @param data The block of data to write to the file.
 */
void writeBinFile(const std::vector<uint32_t>& data) {

    std::map<std::string, std::string> config = loadConfig();
    int runNumber = std::stoi(config["run_number"]);
    // Get filename as char*
    char* filename = getRunFilename(runNumber, config["daq_path"]+config["data_path"], config["file_prefix"]);
    // log->info("Starting run {0:d}, saving data to: {1}", runNumber, filename);

    FILE* file = fopen(filename, "ab");
    // fwrite(data.data(), 1, data.size(), file);
    fwrite(data.data(), sizeof(uint32_t), data.size(), file);
    fclose(file);
    free(filename);
}

/**
 * @brief A thread that writes binary data to a file.
 *
 * This function is launched in a separate thread and waits for data blocks
 * to appear in the block queue. Once a block is available, it is written to
 * a binary file associated with the current run number.
 *
 * @note This function will only terminate once the readout has been stopped
 *       using the `stop_run` function.
 */
void fileWriterThread() {
    auto log = Logger::getLogger();
    while(true) {
        log->debug("fileWriterThread started");
        std::unique_lock<std::mutex> lock(blockQueueMutex);
        blockQueueCond.wait(lock, [] { return !blockQueue.empty() || stopWriter; });

        if (blockQueue.empty() && stopWriter) {
            break;
        }

        std::vector<uint32_t> binaryData = std::move(blockQueue.front());
        blockQueue.pop();
        lock.unlock();

        writeBinFile(binaryData);  // Function to write data to file
    }
}



/**
 * @brief Processes events from the bank queue and prepares them for writing.
 * 
 * This function continuously monitors the bank queue for available data
 * banks while the readout is active. It collects data banks into a block,
 * adds additional GATE and CUSP data banks with information on the run number,
 * and serializes the block for writing. The serialized block is then
 * added to the block queue for further processing.
 * 
 * The function will terminate once the readout is stopped by setting
 * `stopReadout` to true.
 */

  void processEvents() {
    while(!stopReadout) {

        Block block(blockID);
        std::map<std::string, std::string> config = loadConfig();
        

        DataBank CUSP("CUSP");        // Adding the current s timestamp to the CUSP bank
        auto now = std::chrono::system_clock::now();
        uint32_t now_s = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
        std::ifstream cuspFile(config["daq_path"]+"CUSP/Number.txt");
        if (cuspFile) {
            int cuspValue;
            cuspFile >> cuspValue;
            Event eventCUSPrun;
            eventCUSPrun.data.push_back(cuspValue);
            eventCUSPrun.timestamp = now_s;
            CUSP.addEvent(eventCUSPrun);
        }
        block.addDataBank(CUSP);

        std::unique_lock<std::mutex> lock(bankQueueMutex);
        dataAvailable.wait(lock, [] {return !bankQueue.empty() || stopReadout; });

        if (bankQueue.empty() && stopReadout) break;

        

        while (!bankQueue.empty()) {
            DataBank dataBank = std::move(bankQueue.front());
            bankQueue.pop();
            block.addDataBank(dataBank);

        }

        lock.unlock();

      // DataBank GATE("GATE");
      // fpgas[0]->readFIFO(GATE, 0x0000);   // This function will only be properly written once I have a working Gate Register on the V2495
      // block.addDataBank(GATE);



        std::vector<uint32_t> binaryData = block.serialize();
        {
            std::lock_guard<std::mutex> blockLock(blockQueueMutex);
            blockQueue.push(std::move(binaryData));
        }
        blockQueueCond.notify_one();

        blockID++;

  }
}

/**
 * @brief Monitors the TDCs and initiates readout when almost full.
 *
 * This function continuously checks the status of all TDCs to determine if any
 * of them are almost full. If a TDC is almost full, it starts a readout thread
 * for all TDCs to prevent data loss. The function runs while the readout is
 * active and pauses briefly between checks to reduce CPU usage.
 *
 * The function terminates once `stopReadout` is set to true.
 */

void polling() {
    auto log = Logger::getLogger();
    log->debug("Polling thread started");

    try {
        bool isfull = false;

        while (!stopReadout) {
            isfull = false;

            for (auto tdc : tdcs) {
                isfull |= tdc->almostFull();
            }

            if (isfull) {

                for (int i = 0; i < NUM_TDCS; i++){
                    if (!tdcReading[i]) {
                        // log->debug("Starting readout thread {:d}", i);
                        tdcReading[i] = true;
                        startReadoutThread(*tdcs[i], "TDC"+std::to_string(i));
                        
                        log->debug("Readout thread started {:d}", i);
                    }
                }
                isfull = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Small delay to avoid overloading CPU
                for (auto& thread : readoutThreads) {
                    if (thread.joinable()) {
                        thread.join();
                    }
                }
                readoutThreads.clear();
                readoutThreads.resize(NUM_TDCS);
                
            }
            
            




        }

    } catch (const std::exception& ex) {
        log->error("Exception in polling thread: {}", ex.what());
    } catch (...) {
        log->error("Unknown exception in polling thread!");
    }

}



/**
 * @brief Starts a new run by initializing the TDCs and starting the polling thread.
 *
 * This function starts a new run by incrementing the run number, saving it to
 * the configuration file, and initializing all TDCs and the FPGA. It also starts the
 * polling thread which checks the status of all TDCs to determine if any
 * of them are almost full. If a TDC is almost full, it starts a readout thread
 * for all TDCs to prevent data loss.
 *
 * @return true if the run was started successfully, false otherwise.
 */
bool start_run() {

    auto log = Logger::getLogger();

    log->info("Starting data acquisition...");

    is_running = true;
    
    std::map<std::string, std::string> config = loadConfig();
    int runNumber = std::stoi(config["run_number"]) +1;
    config["run_number"] = std::to_string(runNumber);
    saveConfig(config);  // Save updated run number

  // Get filename as char*
    // char* filename = getRunFilename(runNumber, config["data_path"], config["file_prefix"]);
    log->info("Starting run {0:d}", runNumber);

    for (auto tdc : tdcs) {
        if (!tdc->start()) { 
            // std::cerr << "Failed to start acquisition on TDC!" << std::endl;
            log->error("Failed to start data acquisition on TDC!");
            return false;
        }
    }

    blockID = 0;

    // for (auto fpga : fpgas) {
    //   if (!fpga->start()) {

    //   }
    // }

    try {
        stopReadout = false;
        stopWriter = false;
        pollingThread = std::thread(polling);
        processingThread = std::thread(processEvents);
        fileWriter = std::thread(fileWriterThread);
    } catch (const std::exception& e) {
        log->error("Failed to create polling thread: {}", e.what());
        return false;  // Thread creation failed
    }

    vme.stopVeto();
    log->info("Data acquisition started!");

    return true;
}

/**
 * @brief Stops the data acquisition process and finalizes the readout.
 *
 * This function halts the data acquisition by stopping all TDCs and
 * signaling the termination of ongoing readout and writing operations.
 * It notifies waiting threads to proceed, joins all active threads, and
 * ensures that any remaining data in the queues is processed and written
 * to the output files. The function also performs a final readout of the
 * TDCs to capture any last bits of data before concluding the run.
 */

void stop_run() {
    auto log = Logger::getLogger();

    if (is_running) {
        
        log->info("Stopping data acquisition...");

        for (auto tdc : tdcs) {
            tdc->stop(); 
        }

        stopReadout = true;
        dataAvailable.notify_all();
        blockQueueCond.notify_all();

        if (pollingThread.joinable()) {
            pollingThread.join();  // Wait for polling thread to finish
        }
        if (processingThread.joinable()) {
            processingThread.join();
        }

        for (auto& thread : readoutThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        readoutThreads.clear();

        log->debug("Forcing final readout of TDCs");

        for (int i = 0; i < NUM_TDCS; i++) {
            DataBank lastBank(("TDC" + std::to_string(i)).c_str());
            unsigned int wordsRead = tdcs[i]->BLTRead(lastBank);

            if (wordsRead > 0) {
                std::lock_guard<std::mutex> lock(bankQueueMutex);
                bankQueue.push(std::move(lastBank));
                dataAvailable.notify_one();
            }
        }

        if (!bankQueue.empty()) {

            log->debug("Flushing remaining data...");

            Block finalBlock(blockID);

            DataBank CUSP("CUSP");        
            auto now = std::chrono::system_clock::now();
            uint32_t now_s = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
            std::ifstream cuspFile("/home/hododaq/hodo_daq/CUSP/Number.txt");
            if (cuspFile) {
                int cuspValue;
                cuspFile >> cuspValue;
                Event eventCUSPrun;
                eventCUSPrun.data.push_back(cuspValue);
                eventCUSPrun.timestamp = now_s;
                CUSP.addEvent(eventCUSPrun);
            }
            finalBlock.addDataBank(CUSP);

            std::unique_lock<std::mutex> lock(bankQueueMutex);
            while (!bankQueue.empty()) {
                DataBank dataBank = std::move(bankQueue.front());
                bankQueue.pop();
                lock.unlock();

                finalBlock.addDataBank(dataBank);
                lock.lock();
            }
            lock.unlock();

            // Also add the last GATE and CUSP banks
            //DataBank GATE("GATE");
            //fpgas[0]->readFIFO(GATE, 0x0000);  
            //finalBlock.addDataBank(GATE);


            // Serialize and push to file writer queue
            std::vector<uint32_t> binaryData = finalBlock.serialize();
            {
                std::lock_guard<std::mutex> blockLock(blockQueueMutex);
                blockQueue.push(std::move(binaryData));
            }
            blockQueueCond.notify_one();

            blockID++; // Increment block ID
        }

        stopWriter = true;
        blockQueueCond.notify_all();
        if (fileWriter.joinable()) {
            fileWriter.join();
        }

        
        log->info("Data acquisition stopped");

        is_running = false;

    } else {
        log->error("Nothing is running, so it can't be stopped.");
    }

}

/**
 * @brief Pauses the data acquisition.
 *
 * This function will pause the data acquisition by sending a software
 * trigger to the VME controller, which will then pause the run.
 *
 * @return true on success, false otherwise.
 */
bool pause_run() {
    return vme.startVeto();
}

/**
 * @brief Resumes the data acquisition.
 *
 * This function will resume the data acquisition by clearing the software
 * trigger in the VME controller, which will then resume the run.
 *
 * @return true on success, false otherwise.
 */
bool resume_run() {
    return vme.stopVeto();
}


/**
 * @brief Cleans up resources and exits the DAQ.
 *
 * This function deletes all allocated TDC and FPGA objects, and closes the VME interface.
 * It ensures that all dynamically allocated resources are properly released before exiting.
 *
 * @return 0 indicating successful cleanup and exit.
 */

int daq_exit() {

    auto log = Logger::getLogger();
    log->info("Deleting daq_control");

    for(int i=0; i<NUM_TDCS; i++){
        delete tdcs[i];
    }
    for(int i=0; i<NUM_FPGAS; i++){
        delete fpgas[i];  
    }
    vme.close();
    exit(0);
    return 0;
}

bool hardware_inits() {

    auto log = Logger::getLogger();
    bool success = true;
    log->debug("Starting VME initialization...");
    if (handle == -1) {
        vme.init();
        log->debug("VME init done");
        handle = vme.getHandle();
        log->debug("Got VME handle: {0:d}", handle);
        vme.startVeto();

    }


    // Connect to all TDCs:
    for(int i=0; i<NUM_TDCS; i++){
        tdcs[i] = new v1190(TDCbaseAddresses[i], handle);
    }

    for(int i=0; i<NUM_TDCS; i++){
        V1190Status[i] = true;
        // V1190Status[i] = tdcs[i]->checkModuleResponse(); // this is already called in the init function
        V1190Status[i] &= tdcs[i]->init(i);

        if (V1190Status[i]) {
            log->info("V1190 module {0:d} is online", i);
        } else {
            log->warn("V1190 module {0:d} is NOT online!", i);
            success = false;
        }
    }



    for(int i=0; i<NUM_FPGAS; i++){
        fpgas[i] = new v2495((char*)FPGAbaseAddress, handle);
    }

    for(int i=0; i<NUM_FPGAS; i++){
        V2495Status[i] = fpgas[i]->init(i); // Opens connection

        if (V2495Status[i] == 0) {
            log->info("V2495 module {0:d} is online", i);
        } else {
          log->info("V2495 module {0:d} is NOT online!", i);
          success = false;
        }

    }

    return success;
}


/**
 * @brief The main entry point of the DAQ application.
 *
 * This function initializes the DAQ, loads the configuration file,
 * connects to the VME interface, and initializes all connected TDCs and
 * FPGAs. 
 *
 * @return 0 indicating successful execution.
 */
int main() {

    Logger::init();
    auto log = Logger::getLogger();

    log->info("Hiya!");
    log->debug("Oy!");

    TCPServer server(12345);  // Choose a port
    server.start();

    all_init = false;

    while (!hardware_inits()) {
        log->warn("Hardware initialization failed. Retrying...");
        std::this_thread::sleep_for(std::chrono::seconds(5)); 
    }

    all_init = true;

    std::map<std::string, std::string> config = loadConfig();

    int runNumber = std::stoi(config["run_number"]);
    log->info("Current run number: {0:d}", runNumber);

    while (server.isRunning()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    
    return 0;
}