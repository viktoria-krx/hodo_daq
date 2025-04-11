#include <iostream>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include "v2495.hh"

v2495::v2495(int ConnType, char* IpAddr, int SerialNumber, char* vmeBaseAddress, int handle) 
    : ConnType(ConnType), IpAddr(IpAddr), vmeBaseAddress(vmeBaseAddress), handle(handle) {

}

int v2495::init(int fpgaId){

    auto log = Logger::getLogger();

    // printf("  Trying to connect to the V2495 module\n");
    log->info("Trying to connect to the V2495 module");

    switch (ConnType) {
        case 4: // ETH V4718
            ret = CAEN_PLU_OpenDevice2(CAEN_PLU_CONNECT_VME_V4718_ETH, IpAddr, 0, vmeBaseAddress, &handle);
            break;
        case 5: // USB V4718
            ret = CAEN_PLU_OpenDevice2(CAEN_PLU_CONNECT_VME_V4718_USB, &SerialNumber, 0, vmeBaseAddress, &handle);
            break;
        case 6: // USB_A4818
            ret = CAEN_PLU_OpenDevice2(CAEN_PLU_CONNECT_VME_A4818, &SerialNumber, 0, vmeBaseAddress, &handle);
            break;
        default:
            break;
	}

    if (ret != 0) {
		// printf("  The V2495 module cannot be accessed!\n");
        log->error("The V2495 module cannot be accessed!");
    } else {
        // printf("  A connection with the V2495 module has been established\n");
        log->info("A connection with the V2495 module has been established");
    }

    if (CAEN_PLU_ReadReg(handle, MAIN_FIRMWARE_REVISION, data) < 0) {
			// printf("ERROR: the main firmware revision cannot be read\n");
            log->error("The main firmware revision cannot be read");
			return -1;
	}

    return (int)ret;

}

int v2495::close(int fpgaId){

    ret = CAEN_PLU_CloseDevice(handle);

    return (int)ret;

}

uint64_t v2495::getBaseAddr() {
    uint64_t baseAddr = (uint64_t)strtoul(vmeBaseAddress, nullptr, 16);

    return baseAddr;
}

int v2495::readRegister(DataBank& dataBank, uint32_t regAddress){

    addr = getBaseAddr() + regAddress;
    CAEN_PLU_ReadReg(handle, addr, data);

    return 0;

}


int v2495::setRegister(uint32_t value, uint32_t regAddress){

    addr = getBaseAddr() + regAddress;
    CAEN_PLU_WriteReg(handle, addr, value);

    return 0;

}

int v2495::readFIFO(DataBank& dataBank, uint32_t regAddress) {

    auto log = Logger::getLogger();

    unsigned int* buff = NULL;
    int BufferSize;

    BufferSize = 1024 * 1024;
	if ((buff = (unsigned int*)malloc(BufferSize)) == NULL) {
		// printf("Can't allocate memory buffer of %d KB\n", BufferSize / 1024);
        
        log->error("Can't allocate memory buffer of {:d} KB\n", BufferSize / 1024);
	}

    addr = getBaseAddr() + regAddress;

    uint32_t n_words = 0; 

    int ret = CAEN_PLU_ReadFIFO32(handle, addr, BufferSize, buff, &n_words);

    if (ret != cvSuccess && ret != cvBusError) {
        // std::cerr << "FIFO Readout Error" << std::endl;
        log->error("FIFO Readout Error");
        return ret;
    }

    // printf("%i words read\n", n_words);
    log->debug("{:d} words read", n_words);


    Event currentEvent;
    for (int i = 0; i < (int)n_words; i++) {
        uint32_t word = buff[i];

        // // Check for Global Header (start of event)
        // if (IS_GLOBAL_HEADER(word)) {  
        //     if (!currentEvent.data.empty()) {
        //         dataBank.addEvent(currentEvent);
        //         currentEvent.data.clear();
        //     }
        //     currentEvent.eventID = DATA_EVENT_ID(word);  // Extract event ID
        //     currentEvent.data.push_back(word);
        // }
        // else if (IS_TDC_HEADER(word)) {
        //     currentEvent.timestamp = DATA_BUNCH_ID(word);
        //     currentEvent.data.push_back(word);
        // }
        // // Check for Global Trailer (end of event)
        // else if (IS_GLOBAL_TRAILER(word)) {
        //     currentEvent.data.push_back(word);
        //     dataBank.addEvent(currentEvent);
        //     currentEvent.data.clear();
        // }
        // // Regular data word
        // else {
        //     currentEvent.data.push_back(word);
        // }
    }

    free(buff);
    return n_words;

}

