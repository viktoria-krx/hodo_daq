#include "vmeInterface.hh"
#include <iostream>
#include <CAENVMElib.h>
#include <unistd.h>

#define Sleep(x) usleep((x)*1000)

// VMEInterface::VMEInterface(uint32_t vmeBaseAddress) : vmeBaseAddress(vmeBaseAddress), handle(-1) {}

VMEInterface::VMEInterface(int ConnType, char* vmeIPAddress) 
    : ConnType(ConnType), vmeIPAddress(vmeIPAddress) {

}


VMEInterface::~VMEInterface() {
    close();
}

/**
 * @brief Initializes the VME interface.
 *
 * This function attempts to open the VME controller using the configured
 * base address. If the initialization is successful, it also sets up the
 * veto bits. If the initialization fails, it logs an error and sleeps for
 * 1 second before returning.
 *
 * @return true on success, false on failure.
 */
bool VMEInterface::init() {

    auto log = Logger::getLogger();

    // char ip[24];
    // uint32_t link = 0;
    CVBoardTypes BType = cvETH_V4718;
    // call VME Init
    // void* arg = BType == cvETH_V4718_LOCAL ? (void*)vmeIPAddress : (void*)&link;
    // CAENVME_Init2(BType, vmeIPAddress, 0, &handle);
    if (CAENVME_Init2(BType, vmeIPAddress, 0, &handle) != cvSuccess) {
        // printf("Can't open VME controller\n");

        log->error("Can't open VME controller");
        return false;
    } else {
        log->debug("VME init succeeded");
        log->info("VME handle: {0:d}", handle);
    }



    if (setupVeto() != cvSuccess) {
        log->error("Can't set up veto");
    }
    if (stopVeto() != cvSuccess) {
        log->error("Can't stop veto");
    } else {
        log->debug("Veto stopped, sleeping 0.5 s.");
        sleep(0.5);
    }

    if (startVeto() != cvSuccess) {
        log->error("Can't start veto");
    } else {
        log->debug("Veto started, sleeping 1 s.");
        sleep(1);
    }    

    if (stopVeto() != cvSuccess) {
        log->error("Can't stop veto");
    } else {
        log->debug("Veto stopped.");
    }


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

    /**
     * @brief Configures the VME interface for vetoing.
     *
     * This function configures the VME interface to enable vetoing. It sets the
     * output to a constant Pulser A output.
     *
     * @return true if the configuration is successful, false otherwise.
     */
int VMEInterface::setupVeto() {
    // bool ret = true;
    // ret = write(OUT_2_0_MUX_SET, 0x205);    // 0101 = 5 = Pulser A Output, 0 = Data Strobe Signal, 2 = Data Acknowledge Signal
    // ret &= write(PULSE_A_SETUP, 0x0);       
    // ret &= write(PULSE_A_WIDTH, 0x0000);    // 0 = Constant output signal

	//Pulser parametes
	int re = 0;
	unsigned char Period = 0x0000;
	unsigned char Width = 0x0000;

    re = CAENVME_SetPulserConf(handle, cvPulserA, Period,
                      Width, cvUnit25ns, Width,
                      cvManualSW, cvManualSW);

    if (re != cvSuccess) {
        return re;
    }
    re = CAENVME_SetOutputConf(handle, cvOutput1, cvDirect,
                      cvActiveHigh, cvPulserV3718A);



    return re;
}

    /**
     * @brief Starts the VME veto signal.
     *
     * This function starts the generation of the veto signal by the VME
     * interface. It sets the "SW trigger" bit in the Pulser A start register.
     *
     * @return true if the veto signal is started successfully, false otherwise.
     */
int VMEInterface::startVeto() {
    // bool ret = true;
    // ret = write(PULSE_A_START, 0x1);    // bit 1 is SW trigger
    // return ret;
    int re = 0;
    re = CAENVME_StartPulser(handle, cvPulserA);
    return re;
}

/**
 * @brief Stops the VME veto signal.
 *
 * This function stops the generation of the veto signal by the VME
 * interface. It clears the "SW trigger" bit in the Pulser A start 
 * register and issues a clear command to the Pulser A clear register.
 *
 * @return true if the veto signal is stopped successfully, false otherwise.
 */

int VMEInterface::stopVeto() {
    // bool ret = true;
    // ret = write(PULSE_A_START, 0x0);    // bit 1 is SW trigger
    // ret &= write(PULSE_A_CLEAR, 0x1);    // bit 1 is SW clear
    // ret &= write(PULSE_A_CLEAR, 0x0);    // bit 1 is SW clear
    // return ret;
    int re = 0;
    re = CAENVME_StopPulser(handle, cvPulserA);
    return re;

}

