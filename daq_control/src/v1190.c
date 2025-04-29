#include <CAENVMElib.h>
#include "v1190.h"
#include <unistd.h>

#define Sleep(x) usleep((x)*1000)

int VMEerror = 0;

// ---------------------------------------------------------------------------
// Read a 16 bit register of the V1190
// ---------------------------------------------------------------------------
unsigned short V1190ReadRegister(unsigned short RegAddr, int handle, int BaseAddress)
{
	unsigned short reg = 0;
	VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + RegAddr, &reg, cvA32_U_DATA, cvD16);
	return reg;
}

// ---------------------------------------------------------------------------
// Write a 16 bit register of the V1190
// ---------------------------------------------------------------------------
void V1190WriteRegister(unsigned short RegAddr, unsigned short RegData, int handle, int BaseAddress)
{
	unsigned short reg = RegData;
	VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + RegAddr, &reg, cvA32_U_DATA, cvD16);
}

// ---------------------------------------------------------------------------
// Write Opcode
// ---------------------------------------------------------------------------
int V1190WriteOpcode(int nw, const unsigned short* Data, int handle, int BaseAddress)
{
	int i, timeout = 0;
	unsigned short hs = 0;

	for (i = 0; i < nw; i++) {
		do {
			hs = V1190ReadRegister(PROG_HS, handle, BaseAddress);
			timeout++;
			Sleep(1);
		} while (!VMEerror && ((hs & 0x01) == 0) && (timeout < 3000)); /* wait to write */
		if ((timeout == 3000) || VMEerror)
			return 1;
		V1190WriteRegister(OPCODE, Data[i], handle, BaseAddress);
		if (VMEerror) {
			return 1;
		}
	}
	return 0;
}

void V1190SoftClear(unsigned short RegData, int handle, int BaseAddress)
{
    unsigned short reg = RegData;
    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + SW_CLEAR, &reg, cvA32_U_DATA, cvD16);
}

void V1190SoftReset(unsigned short RegData, int handle, int BaseAddress)
{
    unsigned short reg = RegData;
    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + SW_RESET, &reg, cvA32_U_DATA, cvD16);
}

unsigned short V1190ReadControlRegister(int handle, int BaseAddress)
{
    unsigned short reg = 0;
	VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + CONTROL, &reg, cvA32_U_DATA, cvD16);
	return reg;
}

// unsigned short V1190WriteControlRegister()
// {
//     unsigned short reg;
// 	VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + CONTROL, &reg, cvA32_U_DATA, cvD16);
// 	return reg;
// }
int V1190_EnableFIFO(int handle, int BaseAddress) {
    int cr_fifo; 
    cr_fifo = V1190ReadControlRegister(handle, BaseAddress);

    int controlValue = cr_fifo | FIFO_ENABLE_MASK; 

    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + CONTROL, &controlValue, cvA32_U_DATA, cvD16);

    return controlValue; 

}

int V1190_EnableETTT(int handle, int BaseAddress) {
    int cr_ettt; 
    cr_ettt = V1190ReadControlRegister(handle, BaseAddress);

    int controlValue = cr_ettt | ETTT_ENABLE_MASK; // Set bits 9 and 11

    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + CONTROL, 
                                &controlValue, cvA32_U_DATA, cvD16);
    return controlValue;  
}

int V1190_EnableBERR(int handle, int BaseAddress) {
    int cr_berr; 
    cr_berr = V1190ReadControlRegister(handle, BaseAddress);

    int controlValue = cr_berr | BERR_ENABLE_MASK; // Set bit 1

    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + CONTROL, 
                                &controlValue, cvA32_U_DATA, cvD16);
    return controlValue;  
}

int V1190SetBltEvtNr(unsigned short RegData, int handle, int BaseAddress)
{
    unsigned short reg = RegData;
    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + BLT_EVNUM, &reg, cvA32_U_DATA, cvD16);

    return VMEerror;
}

void V1190SetAlmostFullLevel(unsigned short RegData, int handle, int BaseAddress)
{
    unsigned short reg = RegData;
    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + AF_LEV, &reg, cvA32_U_DATA, cvD16);
}

void V1190WriteDummyValue(unsigned short RegData, int handle, int BaseAddress)
{
    unsigned short reg = RegData;
    VMEerror |= CAENVME_WriteCycle(handle, BaseAddress + DUMMY, &reg, cvA32_U_DATA, cvD16);
}

unsigned short V1190ReadDummyValue(int handle, int BaseAddress)
{
    unsigned short reg = 0;
	VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + DUMMY, &reg, cvA32_U_DATA, cvD16);
	return reg;
}

unsigned short V1190ReadFirmwareRevision(int handle, int BaseAddress)
{
    unsigned short reg = 0;
	VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + FW_REVISION, &reg, cvA32_U_DATA, cvD16);
	return reg;
}

unsigned short V1190ReadStatusRegister(int handle, int BaseAddress)
{
    unsigned short reg = 0;
    VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + STATUS, &reg, cvA32_U_DATA, cvD16);
    return reg;
}

unsigned short V1190DataReady(int handle, int BaseAddress)
{
    unsigned short status = V1190ReadStatusRegister(handle, BaseAddress);
    return (status & 0x0001);
}

unsigned short V1190AlmostFull(int handle, int BaseAddress)
{
    unsigned short status = V1190ReadStatusRegister(handle, BaseAddress);
    return (status & 0x0002) >> 1;
}

int V1190EventStored(int handle, int BaseAddress)
{
    unsigned short reg = 0;
    VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + EV_STORED, &reg, cvA32_U_DATA, cvD16);
    return (int)reg;
}

unsigned short V1190GetFIFOWordCount(int handle, int BaseAddress)
{
    unsigned int reg = 0;
    VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + EV_FIFO, &reg, cvA32_U_DATA, cvD32);
    return (unsigned short)(reg & 0xffff);
}

unsigned int V1190GetEventCounter(int handle, int BaseAddress)
{
    unsigned int reg = 0;
    VMEerror |= CAENVME_ReadCycle(handle, BaseAddress + EV_CNT, &reg, cvA32_U_DATA, cvD32);
    return reg;
}


unsigned int V1190BLTRead(unsigned int *buffer, int BufferSize, int* nb, int handle, int BaseAddress)
{
    unsigned int ret = 0;
    ret = CAENVME_FIFOBLTReadCycle(handle, BaseAddress, (unsigned char*)buffer, BufferSize, cvA32_U_MBLT, cvD64, nb);

    return ret;
}
