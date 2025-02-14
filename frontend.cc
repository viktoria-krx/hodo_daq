#include "v1190.hh"
#include "v2495.hh"
#include "dataBanks.hh"
#include "vmeInterface.hh"
#include <iostream>
#include <vector>
#include <memory>
#include <stdlib.h>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

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
std::queue<Event> eventQueue;
std::mutex queueMutex;
std::condition_variable dataAvailable;
bool stopReadout = false;


int main() {
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
      printf("\nV1190 module %d is online\n", i);
    } else {
      printf("\nV1190 module %d is NOT online!\n", i);
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


bool start_run() {
    std::cout << "Starting data acquisition..." << std::endl;

    for (auto tdc : tdcs) {
        if (!tdc->start()) { // Assuming a method to start DAQ
            std::cerr << "Failed to start acquisition on TDC!" << std::endl;
            return false;
        }
    }

    // for (auto fpga : fpgas) {
    //   if (!fpga->start()) {

    //   }
    // }

    vme.stopVeto();
    std::cout << "Data acquisition started!" << std::endl;
    return true;
}

void stop_run() {
    std::cout << "Stopping data acquisition..." << std::endl;

    for (auto tdc : tdcs) {
        tdc->stop(); // Assuming a method to stop DAQ
    }

    std::cout << "Data acquisition stopped!" << std::endl;
}

bool pause_run() {
  return vme.startVeto();
}

bool resume_run() {
  return vme.stopVeto();
}


void readoutLoopTDC(v1190& tdc, DataBank& dataBank) {
  while (!stopReadout) {
    unsigned int wordsRead = tdc.BLTRead(dataBank);
    if (wordsRead > 0) {
      std::lock_guard<std::mutex> lock(queueMutex);
      dataAvailable.notify_one();
    }
  }
}

void processEvents() {
  while(!stopReadout) {
    std::unique_lock<std::mutex> lock(queueMutex);
    dataAvailable.wait(lock, [] {return !eventQueue.empty() || stopReadout; });

    if (stopReadout) break;

    Event event = eventQueue.front();
    eventQueue.pop();
    lock.unlock();

    //Process event

  }
}