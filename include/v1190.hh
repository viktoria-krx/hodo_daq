#ifndef __V1190_CC__
#define __V1190_CC__ 

#include "v1190.h"
// #include "vmeInterface.hh"
#include "dataBanks.hh"
#include <CAENVMElib.h>
#include "v1190.h"
#include "dataBanks.hh"
#include "logger.hh"

class v1190 {

public:     
    v1190(int vmeBaseAddress, int handle);

    bool init(int);
    bool setupV1190(int);
    bool checkModuleResponse();
    float getFirmwareRevision();
    bool almostFull();
    unsigned int BLTRead(DataBank& dataBank);
    bool stop();
    bool start();

    void getPara();
    void getStatusReg();

private:

int vmeBaseAddress;
int handle;

unsigned short value;
unsigned short code[10] = { 0 };


};

#endif