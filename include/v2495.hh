#ifndef __V2495_CC__
#define __V2495_CC__

#define MAIN_FIRMWARE_REVISION        0x8200
#define MAIN_FIRMWARE_MEB_USEDW       0x8208 
#define MAIN_SOFT_RESET               0x8218         

// #include "vmeInterface.hh"
#include "dataBanks.hh"
#include <CAENVMElib.h>
#include <CAEN_PLULib.h>
#include "dataBanks.hh"
#include <cstdlib>
#include "logger.hh"

class v2495 {

public:     
    v2495(int ConnType, char* IpAddr, int SerialNumber, char* vmeBaseAddress, int handle);

    int init(int fpgaId);
    int close(int fpgaId);
    int readRegister(DataBank& dataBank, uint32_t regAddress);
    int setRegister(uint32_t value, uint32_t regAddress);
    int readFIFO(DataBank& dataBank, uint32_t regAddress);

    uint64_t getBaseAddr();



private:

char* vmeBaseAddress;
int SerialNumber;
int handle;
char* IpAddr;
int ConnType = 4;
uint32_t addr;

CAEN_PLU_ERROR_CODE ret = CAEN_PLU_GENERIC;

// unsigned short value;
// unsigned short code[10] = { 0 };
uint32_t* data;

};

#endif