/*! 
  \file v1190.cc
  \brief CAEN V1190/V1290 source code
  \author Viktoria Kraxberger <viktoria.kraxberger@oeaw.ac.at>  after Marcus Fleck
  \date 05.02.2025
*/ 

#include <iostream>
#include <time.h>
#include <unistd.h>
#include <chrono>
#include "v1190.hh"


v1190::v1190(int vmeBaseAddress, int handle) 
    : vmeBaseAddress(vmeBaseAddress), handle(handle) {
}

/**
 * @brief Initializes the V1190 module with the given TDC ID.
 *
 * This function first checks the module's response to ensure it is operational.
 * It then sets up the V1190 module using the provided TDC ID, retrieves 
 * parameters, and updates the status register. If any step fails, the 
 * initialization process returns false.
 *
 * @param tdcId The ID of the TDC to initialize.
 * @return True if initialization is successful, false otherwise.
 */

bool v1190::init(int tdcId){

   if(!checkModuleResponse()) return false;

    bool retVal = true;
    retVal &= setupV1190(tdcId);

    getPara();
    getStatusReg();

    return retVal;
}



/**
 * @brief Configures the V1190 module with specific settings for a given TDC.
 *
 * This function sets up the V1190 Time-to-Digital Converter (TDC) module by 
 * configuring its control registers, enabling specific features, and setting 
 * various operational parameters. It performs a board reset, reads the control 
 * register status, enables the ETTT mode, sets the block transfer event number, 
 * and configures the almost full level. It also sets trigger matching flags, 
 * timing window parameters, edge detection, and resolution settings. The 
 * function logs the setup process and returns a boolean indicating success or 
 * failure.
 *
 * @param tdcId The ID of the TDC to configure.
 * @return True if the setup is successful, false otherwise.
 */

bool v1190::setupV1190(int tdcId){

    auto log = Logger::getLogger();

    log->info("TDC {:d} is being set up", tdcId);

    bool retVal = true;

    int status;
    int cr, ettt, al64, blt;
    

    // perform board reset
    V1190SoftClear(0, handle, vmeBaseAddress);
    V1190SoftReset(0, handle, vmeBaseAddress);

    //////////////////////////////////////////////////////////////

    cr = V1190ReadControlRegister(handle, vmeBaseAddress);

    // cr = v1190_EnableFifo(vme, vmeBaseAdress);
    // printf("\n  Control register status (FIFO ON bit6): 0x%04x \n", cr );
    log->debug("Control register status (FIFO ON bit6): {0:#x}", cr);

    //////////////////////////////////////////////////////////////

    ettt = V1190_EnableETTT(handle, vmeBaseAddress);
    // printf("  ETTT enabled             : 0x%08x \n", ettt ); 
    log->debug("ETTT enabled             : {0:#x}", ettt );

    V1190SetBltEvtNr(0x0000, handle, vmeBaseAddress);
    // printf("  BLT set                  : 0x%04x \n", blt );
    log->debug("BLT set                  :{0:#x}", blt );

    //////////////////////////////////////////////////////////////

    unsigned short int almost_full_level = 512;  //Maximum nr of 32-bit words per BLT
    // 16383;  //Maximum nr of 32-bit words per BLT for 1000 Hz
    V1190SetAlmostFullLevel(almost_full_level, handle, vmeBaseAddress);

    code[0] = 0x0000;  // Trigger matching Flag
    if ((status = V1190WriteOpcode(1, code, handle, vmeBaseAddress)) < 0) retVal = false;

    // all values in units of 25 ns (cycle length of the TDCs)

    code[0] = 0x1000;  // Width
    code[1] = 0x38; // 1400 ns
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    code[0] = 0x1100;  // Offset
    code[1] = 0xFFF0; //  -400ns
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);


    code[0] = 0x1400;  // Subtraction flag
    value = V1190WriteOpcode(1, code, handle, vmeBaseAddress); // reference point: beginning of trigger window
    
    // code[0] = 0x1500;  // Disable subtraction flag
    // value = V1190WriteOpcode(1, code); // reference point: last bunch reset

    // Extra search and reject margin = 0 => only events in trigger window recorded
    code[0] = 0x1200;  // Extra search margin
    code[1] = 0x0;  // no extra search margin
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    code[0] = 0x1300;  // reject margin
    code[1] = 0x0;  // no reject margin
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    code[0] = 0x2200;  // Edge Detection
    code[1] = 0x3;  // trailing + leading edge
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    // Resolution 100ps
    code[0] = 0x2400;
    code[1] = 0x0002;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    code[0] = 0x3000;  // TDC Header/Trailer enable
    code[1] = 0x1;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    code[0] = 0x3500;  // TDC Error enable
    // code[0] = 0x3600;  // TDC Error disable
    code[1] = 0.1;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    // code[0] = 0x4200;  // enable all channels
    // code[1] = 0x1;
    // value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    code[0] = 0x4300;  // disable all channels
    code[1] = 0x1;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    log->info("TDC {:d} was set up", tdcId);

    return retVal;
}


/**
 * @brief Checks if the V1190 module is properly responding.
 *
 * Checks if the V1190 module is properly responding by writing a known
 * value to the dummy register and then reading it back. If the read value
 * matches the written value, the module is considered responsive.
 *
 * @return true if module is responsive, false otherwise
 */
bool v1190::checkModuleResponse(){

    auto log = Logger::getLogger();

    int dummy = 0;
    V1190WriteDummyValue(0x1111, handle, vmeBaseAddress);
    dummy = V1190ReadDummyValue(handle, vmeBaseAddress);

    log->debug("Checking response of TDC; dummy == {:#x}, should be 0x1111", dummy);
    // std::cout << "dummy " << dummy << std::endl;
    if (dummy == 0x1111) return true;
    else return false;

}

float v1190::getFirmwareRevision(){

    int csr = (int)V1190ReadFirmwareRevision(handle, vmeBaseAddress);
    int highcsr = (csr & 0xF0) >> 4;
    int lowcsr = csr & 0x0F;

    return (1.0*highcsr + 0.1*lowcsr);
}

/**
 * @brief Check if the V1190 module is almost full.
 *
 * This function checks the status register of the V1190 module and returns
 * true if the module is almost full, false otherwise.
 *
 * @return true if module is almost full, false otherwise
 */
bool v1190::almostFull(){
    unsigned short full = 0;
    full = V1190AlmostFull(handle, vmeBaseAddress);
    return (bool)full;
}

/**
 * @brief Perform a block transfer read (BLT) from the V1190 module.
 *
 * This function reads data from the V1190 module using a block transfer read (BLT)
 * and stores the data in a DataBank object.
 *
 * @param dataBank The DataBank object to store the read data in.
 *
 * @return The number of words read from the module.
 */
unsigned int v1190::BLTRead(DataBank& dataBank) {

    auto log = Logger::getLogger();

    unsigned int* buff = NULL;
    int BufferSize;

    BufferSize = 1024 * 1024;
	if ((buff = (unsigned int*)malloc(BufferSize)) == NULL) {
        log->error("Can't allocate memory buffer of {:d} KB\n", BufferSize / 1024);
		// printf("Can't allocate memory buffer of %d KB\n", BufferSize / 1024);
		return 0;
	}

    int bytesRead = 0; 

    unsigned int ret = V1190BLTRead(buff, BufferSize, bytesRead, handle, vmeBaseAddress);

    if (ret != cvSuccess && ret != cvBusError) {
        log->error("BLT Readout Error");
        // std::cerr << "BLT Readout Error" << std::endl;
        return ret;
    }

    int n_words = bytesRead / 4;
    // printf("%i words read\n", n_words);
    log->debug("{:d} words read", n_words);


    Event currentEvent;
    for (int i = 0; i < n_words; i++) {
        uint32_t word = buff[i];

        // Check for Global Header (start of event)
        if (IS_GLOBAL_HEADER(word)) {  
            if (!currentEvent.data.empty()) {
                dataBank.addEvent(currentEvent);            //  Add completed event
                currentEvent.data.clear();
            }
            currentEvent.eventID = DATA_EVENT_ID(word);     // Extract event ID
            currentEvent.data.push_back(word);
        }
        else if (IS_TDC_HEADER(word)) {
            currentEvent.timestamp = DATA_BUNCH_ID(word);
            currentEvent.data.push_back(word);
        }
        // Check for Global Trailer (end of event)
        else if (IS_GLOBAL_TRAILER(word)) {
            currentEvent.data.push_back(word);
            dataBank.addEvent(currentEvent);                // Add final event
            currentEvent.data.clear();
        }
        // Regular data word
        else {
            currentEvent.data.push_back(word);
        }
    }

    free(buff);
    return n_words;

}




/**
 * @brief Retrieve parameters from V1190 TDC.
 *
 * This function reads the settings from the V1190 TDC and prints them to the
 * log. It reads the match window width, window offset, extra search window
 * width, reject margin, trigger time subtraction, edge detection, and
 * resolution settings.
 */
void v1190::getPara()
{
    int ns = 25;
    unsigned short value;
    unsigned short code[10] = { 0 };

    auto log = Logger::getLogger();

    // cm_msg(MINFO,"vmefrontend","V1190 TDC at VME A24 0x%06x:\n", vmeBaseAdress);

    // WORD code = 0x1600;

    code[0] = 0x1600;
    if ((value = V1190WriteOpcode(1, code, handle, vmeBaseAddress)) < 0) {
        log->error("Couldn't read opcode!");
        // std::cerr << "Couldn't read opcode!" << std::endl;
    }

    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    // printf("  Match Window width       : 0x%04x, %d ns\n", value, value*ns);
    log->info("  Match Window width       : {:#x}, {:d} ns", value, value*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    // printf("  Window offset            : 0x%04x, %d ns\n", value, (value - (0xffff + 0x0001))*ns);
    log->info("  Window offset            : {:#x}, {:d} ns", value, (value - (0xffff + 0x0001))*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    // printf("  Extra Search Window Width: 0x%04x, %d ns\n", value, value*ns);
    log->info("  Extra Search Window Width: {:#x}, {:d} ns", value, value*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    // printf("  Reject Margin            : 0x%04x, %d ns\n", value, value*ns);
    log->info("  Reject Margin            : {:#x}, {:d} ns", value, value*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    // printf("  Trigger Time subtraction : %s\n",(value & 0x1) ? "y" : "n");
    log->info("  Trigger Time subtraction : {:s}",(value & 0x1) ? "y" : "n");

    code[0] = 0x2300;
    value = V1190WriteOpcode(1, code, handle, vmeBaseAddress);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    // printf("  Edge Detection (0:P/1:T/2:L/3:TL) : 0x%02x\n", (value&0x3));
    log->info("  Edge Detection (0:P/1:T/2:L/3:TL) : {:#x}", (value&0x3));

    unsigned short edge = (value&0x3);

    code[0] = 0x2600;
    value = V1190WriteOpcode(1, code, handle, vmeBaseAddress);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    if (edge == 0x00) {
        // printf("  Resolution Edge      : 0x%04x\n", value & 0x0007);
        // printf("  Resolution Width     : 0x%04x\n", (value & 0x0F00) >> 8);

        log->info("  Resolution Edge      : {:#x}", value & 0x0007);
        log->info("  Resolution Width     : {:#x}", (value & 0x0F00) >> 8);

    } else {
        // printf(" Resolution Edges      : 0x%04x\n", value & 0x0003);
        log->info("  Resolution Edges     : {:#x}", value & 0x0003);
    }

}

void v1190::getStatusReg()
{
    auto log = Logger::getLogger();

    int sr;
    sr = V1190ReadStatusRegister(handle, vmeBaseAddress);
    // sr = v1190_GetStatusReg(vme, vmeBaseAdress);
    // cm_msg(MINFO, "vmefrontend", "Status Register: 0x%04X",sr);
    int srFutsui1 = 0x6838;
    int srFutsui2 = 0x6039;
    int srFutsui3 = 0x603B;
    int srFutsui4 = 0x4838;
    int srFutsui5 = 0x4039;
    int srFutsui6 = 0x403B;
    int srFutsui7 = 0x2838;
    int srFutsui8 = 0x2039;
    int srFutsui9 = 0x203B; 
    if((sr != srFutsui1) && (sr != srFutsui2) && (sr != srFutsui3) && (sr != srFutsui4) && (sr != srFutsui5) && (sr != srFutsui6) && (sr != srFutsui7) && (sr != srFutsui8) && (sr != srFutsui9))
    {
        // printf("\nStrange status register! 0x%04X\n",sr);
        log->error("Strange status register! {:#x}",sr);
        // cm_msg(MERROR, "vmefrontend", "Strange status register! 0x%04X",sr);
    }
}


    /**
     * Enable all channels and start the V1190.
     *
     * @return true if successful
     */
bool v1190::start()
{

    code[0] = 0x4200;  // enable all channels
    code[1] = 0x1;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);
    V1190SoftClear(0, handle, vmeBaseAddress);

    return true;
}

    /**
     * Disable all channels and stop the V1190.
     *
     * @return true if successful
     */
bool v1190::stop()
{
    // int read;
    // DataBank dB = ;
    // read = BLTRead(*dB);
    
    code[0] = 0x4400;  // disable all channels
    code[1] = 0x1;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);

    return true;
}