#ifndef __V2495_CC__
#define __V2495_CC__

#define MAIN_FIRMWARE_REVISION      0x8200
#define MAIN_FIRMWARE_MEB_USEDW     0x8208 
#define MAIN_SOFT_RESET             0x8218         
#define BASE_ADDRESS                0x80FC
#define SCRATCH_REGISTER            0x8220

#define SCI_REG_event_cnt           0x1000
#define SCI_REG_reset_cnt           0x1004
#define SCI_REG_freq                0x1008
#define SCI_REG_coincidences_upper  0x100C
#define SCI_REG_coincidences_lower  0x1010
#define SCI_REG_signals_bgo         0x1014
#define SCI_REG_trigger_cnt         0x1018
#define SCI_REG_gated_cnt           0x101C
#define SCI_REG_gate_edge           0x1020
#define SCI_REG_gate_2bit           0x1038
#define SCI_REG_gate_state          0x103C
#define SCI_REG_gate_word           0x1040
#define SCI_REG_dump                0x1054
#define SCI_REG_dump_word           0x1058

#define SCI_REG_Gate_FIFOADDRESS    0x1048
#define SCI_REG_Gate_STATUS         0x104C
#define SCI_REG_Gate_CONFIG         0x1050

#define GATE_EVENT(r)   ((r) & 0x3FFFFFFF)
#define GATE_BOOL(r)    (((r)>>31) & 0x1)
#define DUMP_BOOL(r)    (((r)>>30) & 0x1)



// #include "vmeInterface.hh"
#include "dataBanks.hh"
#include <CAENVMElib.h>
#include <CAEN_PLULib.h>
#include "dataBanks.hh"
#include <cstdlib>
#include "logger.hh"





class v2495 {

public:     
    v2495(char* vmeBaseAddress, int handle);

    int init(int fpgaId);
    int close(int fpgaId);
    int readRegister(uint32_t regAddress);
    int readRegister32(uint32_t regAddress);
    int setRegister(uint32_t value, uint32_t regAddress);
    int startGateList();
    int readList(DataBank& dataBank, uint32_t regAddressList, uint32_t regAddressStatus);
    int resetCounter();
    void WriteDummyValue(unsigned short RegData, int handle, int vmeBaseAddress);
    unsigned short ReadDummyValue(int handle, int vmeBaseAddress);
    bool checkModuleResponse();

    uint64_t getBaseAddr();
    int vmeError = 0;


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