#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************

  CAEN SpA - Viareggio
  www.caen.it

  Program: V1190/V1290 Readout Program
  Date:    11/10/2011
  Author: Carlo Tintori (c.tintori@caen.it)
  
******************************************************************************/
#ifndef V1190_BASIC_H
#define V1190_BASIC_H

//***************************************************************************
// Some register addresses and Macros
//***************************************************************************
#define CONTROL      0x1000
#define STATUS       0x1002
#define INT_LEV      0x100A
#define INT_VECT     0x100C
#define MOD_ID       0x100E
#define MCST_ADDR    0x1010
#define MCST_CTRL    0x1012
#define SW_RESET     0x1014
#define SW_CLEAR     0x1016
#define SW_EV_RESET  0x1018
#define SW_TRIGGER   0x101A
#define EV_CNT       0x101C
#define EV_STORED    0x1020
#define AF_LEV       0x1022
#define BLT_EVNUM    0x1024
#define FW_REVISION  0x1026
#define OPCODE       0x102E
#define PROG_HS      0x1030
#define EV_FIFO      0x1038
#define EV_FIFO_STOR 0x103C
#define EV_FIFO_STAT 0x103E
#define DUMMY        0x1200
#define CR_BOARDID0  0x403C
#define CR_BOARDID1  0x4038
#define CR_BOARDID2  0x4034
#define CR_SERNUM0   0x4084
#define CR_SERNUM1   0x4080
#define CR_SERNUM0_V2   0x4070
#define CR_SERNUM1_V2   0x4074
#define CR_SERNUM2_V2   0x4078
#define CR_SERNUM3_V2   0x407C

#define DATA_TYPE(r)           (((r)>>27) & 0x1F)
#define IS_GLOBAL_HEADER(r)    ((((r)>>27) & 0x1F) == 0x08)  // 01000
#define IS_GLOBAL_TRAILER(r)   ((((r)>>27) & 0x1F) == 0x10)  // 10000
#define IS_TRIGGER_TIME_TAG(r) ((((r)>>27) & 0x1F) == 0x11)  // 10001
#define IS_TDC_DATA(r)         ((((r)>>27) & 0x1F) == 0x00)  // 00000
#define IS_TDC_HEADER(r)       ((((r)>>27) & 0x1F) == 0x01)  // 00001
#define IS_TDC_TRAILER(r)      ((((r)>>27) & 0x1F) == 0x03)  // 00011
#define IS_TDC_ERROR(r)        ((((r)>>27) & 0x1F) == 0x04)  // 00100
#define IS_FILLER(r)           ((((r)>>27) & 0x1F) == 0x18)  // 11000

#define DATA_EVENT_COUNTER(r)  (((r)>>5)  & 0x3FFFFF)
#define DATA_TDC_ID(r)         (((r)>>24) & 0x3)
#define DATA_EVENT_ID(r)       (((r)>>12) & 0xFFF)
#define DATA_BUNCH_ID(r)       ( (r)      & 0xFFF)
#define DATA_CH(r)             (((r)>>19) & 0x7F)
#define DATA_CH_25(r)          (((r)>>21) & 0x1F)
#define DATA_MEAS(r)           ( (r)      & 0x7FFFF)
#define DATA_MEAS_25(r)        ( (r)      & 0x1FFFFF)
#define DATA_TDC_WORD_CNT(r)   (((r)>>4)  & 0xFFFF)

#define ETTT_ENABLE_MASK 0x0A00

unsigned short V1190ReadRegister(unsigned short RegAddr, int handle, int BaseAddress);
void V1190WriteRegister(unsigned short RegAddr, unsigned short RegData, int handle, int BaseAddress);
int V1190WriteOpcode(int nw, const unsigned short* Data, int handle, int BaseAddress);
void V1190SoftClear(unsigned short RegData, int handle, int BaseAddress);
void V1190SoftReset(unsigned short RegData, int handle, int BaseAddress);
unsigned short V1190ReadControlRegister(int handle, int BaseAddress);
unsigned short V1190WriteControlRegister(int handle, int BaseAddress);
void V1190SetBltEvtNr(unsigned short RegData, int handle, int BaseAddress);
void V1190SetAlmostFullLevel(unsigned short RegData, int handle, int BaseAddress);
void V1190WriteDummyValue(unsigned short RegData, int handle, int BaseAddress);
unsigned short V1190ReadDummyValue(int handle, int BaseAddress);
unsigned short V1190ReadFirmwareRevision(int handle, int BaseAddress);
unsigned short V1190ReadStatusRegister(int handle, int BaseAddress);
unsigned short V1190DataReady(int handle, int BaseAddress);
unsigned short V1190AlmostFull(int handle, int BaseAddress);
int V1190EventStored(int handle, int BaseAddress);
unsigned short V1190GetFIFOWordCount(int handle, int BaseAddress);
unsigned int V1190GetEventCounter(int handle, int BaseAddress);
unsigned int V1190BLTRead(unsigned int *buffer, int BufferSize, int nb, int handle, int BaseAddress);
int V1190_EnableETTT(int handle, int BaseAddress);


#endif // V1190_BASIC_H

#ifdef __cplusplus
}
#endif