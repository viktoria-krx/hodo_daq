#include <iostream>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include "v2495.hh"

// v2495::v2495(int ConnType, char* IpAddr, int SerialNumber, char* vmeBaseAddress, int handle) 
//     : ConnType(ConnType), IpAddr(IpAddr), vmeBaseAddress(vmeBaseAddress), handle(handle) { 
// }

v2495::v2495(char* vmeBaseAddress, int handle) 
    : vmeBaseAddress(vmeBaseAddress), handle(handle) {

}

int v2495::init(int fpgaId){

    auto log = Logger::getLogger();

    // printf("  Trying to connect to the V2495 module\n");
    log->info("Trying to connect to the V2495 module");

    if (checkModuleResponse()) {

        log->debug("Resetting Event Counter");
        
        ret = (CAEN_PLU_ERROR_CODE)resetCounter();

        if (ret == cvSuccess) {
            return cvSuccess;
        } else {
            log->error("Counter could not be reset, Error: {:d}", (int)ret);
        }


    } else {
        return cvGenericError;
    }

    // switch (ConnType) {
    //     case 4: // ETH V4718
    //         ret = CAEN_PLU_OpenDevice2(CAEN_PLU_CONNECT_VME_V4718_ETH, IpAddr, 0, vmeBaseAddress, &handle);
    //         break;
    //     case 5: // USB V4718
    //         ret = CAEN_PLU_OpenDevice2(CAEN_PLU_CONNECT_VME_V4718_USB, &SerialNumber, 0, vmeBaseAddress, &handle);
    //         break;
    //     case 6: // USB_A4818
    //         ret = CAEN_PLU_OpenDevice2(CAEN_PLU_CONNECT_VME_A4818, &SerialNumber, 0, vmeBaseAddress, &handle);
    //         break;
    //     default:
    //         break;
	// }

    // if (ret != 0) {
	// 	// printf("  The V2495 module cannot be accessed!\n");
    //     log->error("The V2495 module cannot be accessed!");
    // } else {
    //     // printf("  A connection with the V2495 module has been established\n");
    //     log->info("A connection with the V2495 module has been established");
    // }

    // if (CAEN_PLU_ReadReg(handle, MAIN_FIRMWARE_REVISION, data) < 0) {
	// 		// printf("ERROR: the main firmware revision cannot be read\n");
    //         log->error("The main firmware revision cannot be read");
	// 		return -1;
	// }

    return (int)ret;

}

int v2495::close(int fpgaId){

    ret = CAEN_PLU_CloseDevice(handle);

    return (int)ret;

}

void v2495::WriteDummyValue(unsigned short RegData, int handle, int vmeBaseAddress)
{
    unsigned short reg = RegData;
    vmeError |= CAENVME_WriteCycle(handle, vmeBaseAddress + SCRATCH_REGISTER, &reg, cvA32_U_DATA, cvD16);
}

unsigned short v2495::ReadDummyValue(int handle, int vmeBaseAddress)
{
    unsigned short reg = 0;
	vmeError |= CAENVME_ReadCycle(handle, vmeBaseAddress + SCRATCH_REGISTER, &reg, cvA32_U_DATA, cvD16);
	return reg;
}

bool v2495::checkModuleResponse(){

    auto log = Logger::getLogger();

    int dummy = 0;

    int vmeBA = (int)getBaseAddr();

    WriteDummyValue(0x1111, handle, vmeBA);
    dummy = (int)ReadDummyValue(handle, vmeBA);

    log->debug("Checking response of FPGA; dummy == {:#x}, should be 0x1111", dummy);
    
    if (dummy == (int)0x1111) return true;

    else return false;

}



uint64_t v2495::getBaseAddr() {
    uint64_t baseAddr = (uint64_t)strtoul(vmeBaseAddress, nullptr, 16);

    return baseAddr;
}

int v2495::readRegister(uint32_t regAddress){
    addr = (int)getBaseAddr() + regAddress;
    unsigned short reg = 0;
    vmeError |= CAENVME_ReadCycle(handle, addr, &reg, cvA32_U_DATA, cvD16);
    return reg;
}


int v2495::readRegister32(uint32_t regAddress){
    addr = (int)getBaseAddr() + regAddress;
    uint32_t reg = 0;
    vmeError |= CAENVME_ReadCycle(handle, addr, &reg, cvA32_U_DATA, cvD32);
    return reg;
}

int v2495::setRegister(uint32_t value, uint32_t regAddress){
    addr = (int)getBaseAddr() + regAddress;
    vmeError |= CAENVME_WriteCycle(handle, addr, &value, cvA32_U_DATA, cvD16);
    return vmeError;
}

int v2495::resetCounter() {
    vmeError |= setRegister(0x1, SCI_REG_reset_cnt);
    vmeError |= setRegister(0x0, SCI_REG_reset_cnt);
    return vmeError;
}

int v2495::startGateList() {
    vmeError |= setRegister(0x2, SCI_REG_Gate_CONFIG);
    vmeError |= setRegister(0x1, SCI_REG_Gate_CONFIG);
    return vmeError;
}

int v2495::startTimeList() {
    vmeError |= setRegister(0x2, SCI_REG_TimeTag_CONFIG);
    vmeError |= setRegister(0x1, SCI_REG_TimeTag_CONFIG);
    return vmeError;
}

int v2495::readList(DataBank& dataBank, uint32_t regAddressList, uint32_t regAddressStatus) {

    auto log = Logger::getLogger();
    uint32_t* buff = NULL;
    int BufferSize;
    BufferSize = 1024 * 1024;
	if ((buff = (uint32_t*)malloc(BufferSize)) == NULL) {
        log->error("Can't allocate memory buffer of {:d} KB\n", BufferSize / 1024);
	}
    std::string bankN(dataBank.bankName, 4);

    uint32_t ListStatus = readRegister32(regAddressStatus);
    log->trace("{} List Status: {:b}", bankN, ListStatus);
    uint32_t n_words = (ListStatus & 0xFFFFFF00) >> 8;
    log->trace("{:d} words read from {}", n_words, bankN);
    addr = getBaseAddr() + regAddressList;
    //int n_bytes = 0; 
    // int ret = CAENVME_FIFOBLTReadCycle(handle, addr, buff, BufferSize, cvA32_U_DATA, cvD32, &n_bytes);
    int ret;
//
//    if (ret != cvSuccess && ret != cvBusError) {
//        log->error("FIFO Readout Error");
//        return ret;
//    }
    //int n_words = n_bytes/4;
    // log->trace("{:d} words read", n_words);

    Event currentEvent;
    uint32_t prev_event = -1;
    uint32_t this_event = -1;
    uint32_t word;
    for (int i = 0; i < (int)n_words; i++) {
         // = buff[i];
        ret = CAENVME_ReadCycle(handle, addr, &word, cvA32_U_DATA, cvD32);

        if (ret != cvSuccess && ret != cvBusError) {
            log->error("FIFO Readout Error");
            return ret;
        }


        log->trace("{} List Word: 0x{:x}", bankN, word);
        // log->debug("Gate: {:d}, Event: {:d}", (word & 0x80000000) >> 31, word & 0x7FFFFFFF);
        this_event = GATE_EVENT(word);
        if (currentEvent.eventID == prev_event) continue;
        prev_event = this_event;
        currentEvent.data.push_back(word);
        dataBank.addEvent(currentEvent);
        currentEvent.data.clear();

    }

    free(buff);
    return n_words;

}


// Adapted readList function to read out GATE and timetag simultaneously
int v2495::readTwoLists(DataBank& dataBank, uint32_t regAddressListOne, uint32_t regAddressStatusOne, uint32_t regAddressListTwo, uint32_t regAddressStatusTwo) {

    auto log = Logger::getLogger();
    uint32_t* buffGate = NULL;
    uint64_t* buffTime = NULL;
    int BufferSizeGate;
    BufferSizeGate = 1024 * 1024;
    int BufferSizeTime;
    BufferSizeTime = 1024 * 2024;
	if ((buffGate = (uint32_t*)malloc(BufferSizeGate)) == NULL) {
        log->error("Can't allocate memory buffer of {:d} KB\n", BufferSizeGate / 1024);
	}
    if ((buffTime = (uint64_t*)malloc(BufferSizeTime)) == NULL) {
        log->error("Can't allocate memory buffer of {:d} KB\n", BufferSizeTime / 1024);
	}
    std::string bankN(dataBank.bankName, 4);

    uint32_t ListStatusGate = readRegister32(regAddressStatusOne);
    log->trace("{} List Status: {:b}", bankN, ListStatusGate);
    uint32_t n_wordsGate = (ListStatusGate & 0xFFFFFF00) >> 8;
    uint32_t ListStatusTime = readRegister32(regAddressStatusTwo);
    log->trace("{} List Status: {:b}", bankN, ListStatusTime);
    uint32_t n_wordsTime = (ListStatusTime & 0xFFFFFF00) >> 8;

    log->debug("{:d} words read from {}", n_wordsGate+n_wordsTime, bankN);
    addrGate = getBaseAddr() + regAddressListOne;
    addrTime = getBaseAddr() + regAddressListTwo;
    //int n_bytes = 0; 
    // int ret = CAENVME_FIFOBLTReadCycle(handle, addr, buff, BufferSize, cvA32_U_DATA, cvD32, &n_bytes);
    int retGate, retTime;
//
//    if (ret != cvSuccess && ret != cvBusError) {
//        log->error("FIFO Readout Error");
//        return ret;
//    }
    //int n_words = n_bytes/4;
    // log->trace("{:d} words read", n_words);

    uint32_t n_words = n_wordsGate;

    Event currentEvent;
    uint32_t prev_event = -1;
    uint32_t this_event = -1;
    uint32_t wordGate, wordTimeLow, wordTimeHigh;
    uint64_t wordTime;
    for (int i = 0; i < (int)n_words; i++) {
         // = buff[i];
        retGate = CAENVME_ReadCycle(handle, addrGate, &wordGate, cvA32_U_DATA, cvD32);

        // read two 32-bit words for timestamp (LOW, then HIGH)
        retTime = CAENVME_ReadCycle(handle, addrTime, &wordTimeLow, cvA32_U_DATA, cvD32);
        retTime |= CAENVME_ReadCycle(handle, addrTime, &wordTimeHigh, cvA32_U_DATA, cvD32);

        if (retGate != cvSuccess && retGate != cvBusError && retTime != cvSuccess && retTime != cvBusError) {
            log->error("FIFO Readout Error");
            return ret;
        }

        // combine into 64-bit
        wordTime = (static_cast<uint64_t>(wordTimeHigh) << 32) | wordTimeLow;

        log->trace("{} List Word: 0x{:x}", bankN, wordGate);
        
        // log->debug("Gate: {:d}, Event: {:d}", (word & 0x80000000) >> 31, word & 0x7FFFFFFF);
        this_event = GATE_EVENT(wordGate);
        if (currentEvent.eventID == prev_event) continue;
        prev_event = this_event;
        currentEvent.data.push_back(wordGate);
        currentEvent.timestamp64 = wordTime;
        currentEvent.timestamp = 0;
        // log->debug("{} List time: 0x{:x}", bankN, currentEvent.timestamp);
        // log->debug("{} List time: 0x{:x}", bankN, currentEvent.timestamp64);
        dataBank.addEvent(currentEvent);
        currentEvent.data.clear();

    }

    free(buffGate);
    free(buffTime);
    return n_words;

}


/* int v2495::startCPack(uint32_t regAddress) {
    vmeError |= setRegister(0x1, SCI_REG_Mixing_CONFIG);
    return vmeError;
}*/
