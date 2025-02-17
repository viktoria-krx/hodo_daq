#ifndef VME_INTERFACE_HH
#define VME_INTERFACE_HH

#include <stdint.h>
#include "logger.hh"

#define VME_STATUS      0x00
#define VME_CONTROL     0x01
#define FIRMWARE_REV    0x02
#define IRQ_STATUS      0x05
#define IO_LEVEL        0x07
#define IO_POLARITY     0x08
#define OUT_2_0_MUX_SET 0x09
#define OUT3_MUX_SET    0x0A
#define IO_STATUS_READ  0x0B
#define IO_STATUS_SET   0x0C
#define IO_COINC        0x0D
#define PULSE_A_SETUP   0x10
#define PULSE_A_START   0x11
#define PULSE_A_CLEAR   0x12
#define PULSE_A_NCYCLE  0x13
#define PULSE_A_WIDTH   0x14
#define PULSE_A_DELAY   0x15
#define PULSE_A_PERIOD   0x16
#define PULSE_B_SETUP   0x17
#define PULSE_B_START   0x18
#define PULSE_B_CLEAR   0x19
#define PULSE_B_NCYCLE  0x1A
#define PULSE_B_WIDTH   0x1B
#define PULSE_B_DELAY   0x1C
#define PULSE_B_PERIOD   0x1D


class VMEInterface {
    
public:
    VMEInterface(uint32_t vmeBaseAddress);
    ~VMEInterface();

    bool init();
    bool write(uint32_t offset, uint16_t data);
    bool read(uint32_t offset, uint16_t &data);
    void close();
    
    int getHandle() const { return handle; };

    bool setupVeto();
    bool startVeto();
    bool stopVeto();

private:
int handle;
uint32_t vmeBaseAddress;

};

#endif 