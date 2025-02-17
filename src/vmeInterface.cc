#include "vmeInterface.hh"
#include <iostream>
#include <CAENVMElib.h>
#include <unistd.h>

#define Sleep(x) usleep((x)*1000)

VMEInterface::VMEInterface(uint32_t vmeBaseAddress) : vmeBaseAddress(vmeBaseAddress), handle(-1) {}

VMEInterface::~VMEInterface() {
    close();
}

bool VMEInterface::init() {

    auto log = Logger::getLogger();

    char ip[24];
    uint32_t link = 0;
    CVBoardTypes BType = cvETH_V4718;
    // call VME Init
    void* arg = BType == cvETH_V4718 ? (void*)ip : (void*)&link;
    if (CAENVME_Init2(BType, arg, 0, &handle) != cvSuccess) {
        // printf("Can't open VME controller\n");

        log->error("Can't open VME controller");
        Sleep(1000);
        return false;
    }

    setupVeto();

    // int status = CAENVME_Init2(cvETH_V4718, 0, 0, &handle);
    // if (status != cvSuccess) {
    //     std::cerr << "VME Initialization failed: " << status << std::endl;
    //     return false;
    // }
    return true;
}

bool VMEInterface::write(uint32_t offset, uint16_t data) {
    auto log = Logger::getLogger();
    int status = CAENVME_WriteCycle(handle, vmeBaseAddress + offset, &data, cvA24_U_DATA, cvD16);
    if (status != cvSuccess) {
        // std::cerr << "VME Write failed: " << status << " at offset: " << std::hex << offset << std::endl;
        log->error("VME Write failed: {0:d} at offset {1:#x}", status, offset);
        return false;
    }
    return true;
}

bool VMEInterface::read(uint32_t offset, uint16_t &data) {
    auto log = Logger::getLogger();
    int status = CAENVME_ReadCycle(handle, vmeBaseAddress + offset, &data, cvA24_U_DATA, cvD16);
    if (status != cvSuccess) {
        // std::cerr << "VME Read failed: " << status << " at offset: " << std::hex << offset << std::endl;
        log->error("VME Read failed:  {0:d} at offset {1:#x}", status, offset);
        return false;
    }
    return true;
}

void VMEInterface::close() {
    if (handle >= 0) {
        CAENVME_End(handle);
        handle = -1;
    }
}

bool VMEInterface::setupVeto() {
    bool ret = true;
    ret = write(OUT_2_0_MUX_SET, 0x205);    // 0101 = 5 = Pulser A Output, 0 = Data Strobe Signal, 2 = Data Acknowledge Signal
    ret &= write(PULSE_A_SETUP, 0x0);       
    ret &= write(PULSE_A_WIDTH, 0x0000);    // 0 = Constant output signal

    return ret;
}

bool VMEInterface::startVeto() {
    bool ret = true;
    ret = write(PULSE_A_START, 0x1);    // bit 1 is SW trigger
    return ret;
}

bool VMEInterface::stopVeto() {
    bool ret = true;
    ret = write(PULSE_A_START, 0x0);    // bit 1 is SW trigger
    ret &= write(PULSE_A_CLEAR, 0x1);    // bit 1 is SW clear
    ret &= write(PULSE_A_CLEAR, 0x0);    // bit 1 is SW clear
    return ret;
}