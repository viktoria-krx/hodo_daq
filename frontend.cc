#include "v1190.hh"
#include "v2495.hh"
#include "dataBanks.hh"
#include "vmeInterface.hh"
#include "logger.hh"

#include <iostream>
#include <vector>
#include <memory>
#include <stdlib.h>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>

#define NUM_TDCS 4
#define NUM_FPGAS 1

v1190 *tdcs[NUM_TDCS];
bool V1190Status[NUM_TDCS];
v2495 *fpgas[NUM_FPGAS];
int V2495Status[NUM_FPGAS];

// Define all base addresses:

static const unsigned int BridgeBaseAddress = 0x90000000;

static const unsigned int TDCbaseAddresses[] = {0x90900000, 0x90910000, 0x90920000, 0x90930000};

const char* FPGAbaseAddress = "32100000";
const char* FPGAipAddress = "192.168.1.254";
static const unsigned int FPGAserialNumber = 28;

VMEInterface vme(BridgeBaseAddress);

// Thread safe readout 
std::queue<DataBank> bankQueue;
std::mutex bankQueueMutex;
std::condition_variable dataAvailable;
bool stopReadout = false;

// Thread safe writing
std::queue<std::vector<uint8_t>> blockQueue;
std::mutex blockQueueMutex;
std::condition_variable blockQueueCond;
bool stopWriter = false;

std::vector<std::thread> readoutThreads;

uint32_t blockID = 0;

int main() {

  Logger::init();
  auto log = Logger::getLogger();

  log->info("Hiya!");
  log->debug("Oy!");

  vme.init();

  int handle = vme.getHandle();
  vme.startVeto();
  // Connect to all TDCs:

  for(int i=0; i<NUM_TDCS; i++){
    tdcs[i] = new v1190(TDCbaseAddresses[i], handle);
  }

  for(int i=0; i<NUM_TDCS; i++){
    // V1190Status[i] = tdcs[i]->checkModuleResponse(); // this is already called in the init function
    V1190Status[i] &= tdcs[i]->init(i);

    if (V1190Status[i]) {
      log->info("V1190 module {0:d} is online", i);
      // printf("\nV1190 module %d is online\n", i);
    } else {
      log->warn("V1190 module {0:d} is NOT online!", i);
      // printf("\nV1190 module %d is NOT online!\n", i);
    }
  }

  // Connect to FPGA:
  // ConnType 4 : CAEN_PLU_CONNECT_VME_V4718_ETH
  // ConnType 5 : CAEN_PLU_CONNECT_VME_V4718_USB
  // ConnType 6 : CAEN_PLU_CONNECT_VME_A4818

  for(int i=0; i<NUM_FPGAS; i++){
    fpgas[i] = new v2495(4, (char*)FPGAipAddress, FPGAserialNumber, (char*)FPGAbaseAddress, handle);
  }

  for(int i=0; i<NUM_FPGAS; i++){
    V2495Status[i] = fpgas[i]->init(i); // Opens connection
  }


  return 0;
}

int frontend_exit() {
  for(int i=0; i<NUM_TDCS; i++){
    delete tdcs[i];
  }
  vme.close();
  return 0;
}



void startReadoutThread(v1190& tdc, std::string bankName) {
  readoutThreads.emplace_back([&tdc, bankName] () {
    while(!stopReadout) {
      DataBank dataBank(bankName.c_str());
      unsigned int wordsRead = tdc.BLTRead(dataBank);

      if (wordsRead > 0) {
        std::lock_guard<std::mutex> lock(bankQueueMutex);
        bankQueue.push(std::move(dataBank));
        dataAvailable.notify_one();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
}

void writeBinFile(const std::vector<uint8_t>& data) {
  FILE* file = fopen("output.bin", "ab");
  fwrite(data.data(), 1, data.size(), file);
  fclose(file);
}

void fileWriterThread() {
  while(!stopReadout) {
      std::unique_lock<std::mutex> lock(blockQueueMutex);
      blockQueueCond.wait(lock, [] { return !blockQueue.empty() || stopReadout; });

      std::vector<uint8_t> binaryData = std::move(blockQueue.front());
      blockQueue.pop();
      lock.unlock();

      writeBinFile(binaryData);  // Function to write data to file
  }
}



void processEvents() {
  while(!stopReadout) {
    std::unique_lock<std::mutex> lock(bankQueueMutex);
    dataAvailable.wait(lock, [] {return !bankQueue.empty() || stopReadout; });

    if (stopReadout) break;

    Block block(blockID);

    while (!bankQueue.empty()) {
      DataBank dataBank = std::move(bankQueue.front());
      bankQueue.pop();
      block.addDataBank(dataBank);

    }

      lock.unlock();

    DataBank GATE("GATE");
    fpgas[0]->readFIFO(GATE, 0x0000);   // This function will only be properly written once I have a working Gate Register on the V2495
    block.addDataBank(GATE);

    DataBank CUSP("CUSP");        // Adding the current ms timestamp to the CUSP bank, since this won't change anything
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::ifstream cuspFile("/path/to/cusp.txt");
    if (cuspFile) {
        int cuspValue;
        cuspFile >> cuspValue;
        Event eventCUSPrun;
        eventCUSPrun.data.push_back(cuspValue);
        eventCUSPrun.timestamp = now_ms;
        CUSP.addEvent(eventCUSPrun);
    }

    std::vector<uint8_t> binaryData = block.serialize();
    {
      std::lock_guard<std::mutex> blockLock(blockQueueMutex);
      blockQueue.push(std::move(binaryData));
    }
    blockQueueCond.notify_one();

    blockID++;

  }
}

void polling() {

  bool isfull = false;
  std::vector<bool> tdcReading(NUM_TDCS, false);

  while (!stopReadout) {

    for (auto tdc : tdcs) {
      isfull |= tdc->almostFull();
    }

    if (isfull) {

      for (int i = 0; i < NUM_TDCS; i++){
        if (!tdcReading[i]) {
          startReadoutThread(*tdcs[i], "TDC"+std::to_string(i));
          tdcReading[i] = true;
        }
        
      }

    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Small delay to avoid overloading CPU

  }
}



bool start_run() {

    auto log = Logger::getLogger();

    log->info("Starting data acquisition...");
    // std::cout << "Starting data acquisition..." << std::endl;

    for (auto tdc : tdcs) {
        if (!tdc->start()) { // Assuming a method to start DAQ
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

    vme.stopVeto();
    log->info("Data acquisition started!");
    // std::cout << "Data acquisition started!" << std::endl;
    return true;
}

void stop_run() {

    auto log = Logger::getLogger();

    log->info("Stopping data acquisition...");
    // std::cout << "Stopping data acquisition..." << std::endl;

    for (auto tdc : tdcs) {
        tdc->stop(); 
    }

    stopReadout = true;
    dataAvailable.notify_all();
    blockQueueCond.notify_all();

    for (auto& thread : readoutThreads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    readoutThreads.clear();

    log->debug("Forcing final readout of TDCs");

    // std::cout << "Forcing final readout of TDCs" << std::endl;
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

        // std::cout << "Flushing remaining data..." << std::endl;

        Block finalBlock(blockID);

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
        DataBank GATE("GATE");
        fpgas[0]->readFIFO(GATE, 0x0000);  
        finalBlock.addDataBank(GATE);

        DataBank CUSP("CUSP");        
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::ifstream cuspFile("/path/to/cusp.txt");
        if (cuspFile) {
            int cuspValue;
            cuspFile >> cuspValue;
            Event eventCUSPrun;
            eventCUSPrun.data.push_back(cuspValue);
            eventCUSPrun.timestamp = now_ms;
            CUSP.addEvent(eventCUSPrun);
        }
        finalBlock.addDataBank(CUSP);

        // Serialize and push to file writer queue
        std::vector<uint8_t> binaryData = finalBlock.serialize();
        {
            std::lock_guard<std::mutex> blockLock(blockQueueMutex);
            blockQueue.push(std::move(binaryData));
        }
        blockQueueCond.notify_one();

        blockID++; // Increment block ID
    }

    stopWriter = true;
    blockQueueCond.notify_all();
    fileWriterThread();

    // std::cout << "Data acquisition stopped!" << std::endl;
    log->info("Data acquisition stopped");
}

bool pause_run() {
  return vme.startVeto();
}

bool resume_run() {
  return vme.stopVeto();
}