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

        return cvSuccess;

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
    // std::cout << "dummy " << dummy << std::endl;
    if (dummy == (int)0x1111) return true;

    else return false;

}



uint64_t v2495::getBaseAddr() {
    uint64_t baseAddr = (uint64_t)strtoul(vmeBaseAddress, nullptr, 16);

    return baseAddr;
}

int v2495::readRegister(uint32_t regAddress){

    addr = (int)getBaseAddr(); + regAddress;
    unsigned short reg = 0;
    vmeError |= CAENVME_ReadCycle(handle, addr, &reg, cvA32_U_DATA, cvD16);
    // CAEN_PLU_ReadReg(handle, addr, data);

    return reg;

}


int v2495::setRegister(uint32_t value, uint32_t regAddress){

    addr = (int)getBaseAddr(); + regAddress;
    vmeError |= CAENVME_WriteCycle(handle, addr, &value, cvA32_U_DATA, cvD16);
    // CAEN_PLU_WriteReg(handle, addr, value);

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

    int n_words = 0; 

    // int ret = CAEN_PLU_ReadFIFO32(handle, addr, BufferSize, buff, &n_words);
    int ret = CAENVME_FIFOBLTReadCycle(handle, addr, buff, BufferSize, cvA32_U_DATA, cvD16, &n_words);

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

