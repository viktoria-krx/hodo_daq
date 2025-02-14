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

bool v1190::init(int tdcId){
    if(!checkModuleResponse()) return false;

    bool retVal = true;
    retVal &= setupV1190(tdcId);

    getPara();
    getStatusReg();

    return retVal;
}



bool v1190::setupV1190(int tdcId){

    bool retVal = true;

    int status;
    int cr, ettt, al64, blt;
    

    // perform board reset
    V1190SoftClear(0, handle, vmeBaseAddress);
    V1190SoftReset(0, handle, vmeBaseAddress);

    //////////////////////////////////////////////////////////////

    cr = V1190ReadControlRegister(handle, vmeBaseAddress);

    // cr = v1190_EnableFifo(vme, vmeBaseAdress);
    printf("\n  Control register status (FIFO ON bit6): 0x%04x \n", cr );

    //////////////////////////////////////////////////////////////

    ettt = V1190_EnableETTT(handle, vmeBaseAddress);
    printf("  ETTT enabled             : 0x%08x \n", ettt ); 

    V1190SetBltEvtNr(0x0000, handle, vmeBaseAddress);
    printf("  BLT set                  : 0x%04x \n", blt );

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


    return retVal;
}


bool v1190::checkModuleResponse(){

    int dummy = 0;
    V1190WriteDummyValue(0x1111, handle, vmeBaseAddress);
    dummy = V1190ReadDummyValue(handle, vmeBaseAddress);

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


unsigned int v1190::BLTRead(DataBank& dataBank) {
    unsigned int* buff = NULL;
    int BufferSize;

    BufferSize = 1024 * 1024;
	if ((buff = (unsigned int*)malloc(BufferSize)) == NULL) {
		printf("Can't allocate memory buffer of %d KB\n", BufferSize / 1024);
		return 0;
	}

    int bytesRead = 0; 

    unsigned int ret = V1190BLTRead(buff, BufferSize, bytesRead, handle, vmeBaseAddress);

    if (ret != cvSuccess && ret != cvBusError) {
        std::cerr << "BLT Readout Error" << std::endl;
        return ret;
    }

    int n_words = bytesRead / 4;
    printf("%i words read\n", n_words);


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






// bool v1190::read(void *pevent){
// ////// ADDED NOW   
//     unsigned int* buff = NULL;
//     int BufferSize;
//     int nb;

//     // struct timespec sleeptime;
//     // sleeptime.tv_sec = 0;
//     // sleeptime.tv_nsec = 1000; // unit is nano seconds

//     // getStatusReg();

//     // for(int counter = 0; counter < 200; counter ++){
//     // if(checkEventStatus()) break;
//     // else{
//     //     nanosleep(&sleeptime, NULL);
//     // }
//     // }
//     // if(!checkEventStatus()) return false;
//     // DWORD  data[1000000]; // just a very big buffer...
//     // //DWORD  data[1000]; // just a very big buffer...
//     // char   bname[5];
//     // DWORD *pdata;

//  ////// ADDED NOW   
//     BufferSize = 1024 * 1024;
// 	if ((buff = malloc(BufferSize)) == NULL) {
// 		printf("Can't allocate memory buffer of %d KB\n", BufferSize / 1024);
// 		goto exit_prog;
// 	}



//     ////////////////////////////////////////////Sve
//     //read_BLT32////////////


//     printf("v1190_DataReady: %d\n",v1190_DataReady(vme, vmeBaseAdress));
//     printf("v1190_AlmostFull: %d\n",v1190_IsAlmostFull(vme, vmeBaseAdress));

//     sprintf(bname,"TDC%X",tdcNum);

//     // create bank for TDC data
//     bk_create(pevent, bname, TID_DWORD, &pdata);

//     sleeptime.tv_sec = 0;
//     sleeptime.tv_nsec = 200000; // unit is nano seconds
//     nanosleep(&sleeptime, NULL); 
//     int entries = getEventSize();
//     cm_msg(MINFO, "vmefrontend", "0x%08x events available at TDC", entries);

//     ///////////////////////////////////////////////////////////////// 

//     int n_words=0;

//     if(entries>0){

//         int n_events = v1190_EvtStored(vme, vmeBaseAdress);
//         for (int i=0; i<n_events; i++){

//             n_words+=v1190_GetWrdCntFromEvtFifo(vme, vmeBaseAdress);

//         } 

//         cm_msg(MINFO, "vmefrontend", "number of words in buffer %i total", n_words);

//         int read_level = v1190_ReadAlmostFullLevel(vme, vmeBaseAdress);
//         cm_msg(MINFO, "vmefrontend", "Number of words TDC: %i", read_level);


//         ///////////////////////////////////////////////////////////////
//         //

//         //	v1190_BLTRead(vme, vmeBaseAdress, data, &n_words);// BLT Read Svetlana
//         v1190_BLTRead(vme, vmeBaseAdress, data, n_words);
//         //   v1190_DataRead(vme, vmeBaseAdress, data, (uint32_t)entries);
//         //	 v1190_EventRead(vme, vmeBaseAdress, data, &entries);
//         // v1190_EventReadWCFifo(vme, vmeBaseAdress, data, &entries);  // Read event with WC from FIFO  H.Shi
//         std::chrono::time_point<std::chrono::high_resolution_clock> t_start = std::chrono::high_resolution_clock::now();

//         //std ::cout<<entries<<"entries:"<<std::endl;
//         cm_msg(MINFO, "vmefrontend", "0x%08x words read", entries);
//         for(unsigned int i=0; i<n_words; i++){
//         //std::cout << data[i] << std::endl;
//         *pdata++ = data[i];   
//         /*	std::stringstream sstream;
//         sstream << std::hex << data[i];
//         std::string result = sstream.str();
//         file<<result<<std::endl;*/ //Sve debugging

//         }
//         //file<<"$$$$Nwords$$$"<<n_words<<std::endl;//Sve debugging
//         std::chrono::time_point<std::chrono::high_resolution_clock> t_end = std::chrono::high_resolution_clock::now();


//         std::chrono::duration<float> diff = t_end-t_start;


//         cm_msg(MINFO, "vmefrontend","TDC read time_V1190: %f sec", diff.count() );    
//     } 

//     else {
//         return false;
//     }
 
//     bk_close(pevent,pdata);  
//     //v1190_SoftReset(vme, vmeBaseAdress);
//     v1190_SoftClear(vme, vmeBaseAdress);

//     //cm_msg(MINFO, "vmefrontend", "0x%08x events at TDC after Reset+Clear", entries);
//     // getPara(); // debug for the resolution setting

//     getStatusReg();
//     //entries = getEventSize();

//     return true;
// }

// // void v1190::update(INT hDB, INT hkey, void *dump){
// //     if(dump != NULL){

// //         messenger *mess = (messenger *)dump;

// //         //cm_msg(MINFO,"vmefrontend","Mess: 0x%x 0x%x 0x%x 0x%x",mess->vmeBaseAdress,(uint16_t)(mess->settings->width / 25),(uint16_t)(mess->settings->offset / 25),mess->settings->edge_trailing);//comment

// //         v1190_WidthSet(mess->vme, mess->vmeBaseAdress, (uint16_t)(mess->settings->width / 25));
// //         v1190_OffsetSet(mess->vme, mess->vmeBaseAdress, (uint16_t)(mess->settings->offset / 25));
// //         v1190_SetEdgeDetection(mess->vme, mess->vmeBaseAdress, mess->settings->edge_leading, mess->settings->edge_trailing);
// //     }
// // }

void v1190::getPara()
{
    int ns = 25;
    unsigned short value;
    unsigned short code[10] = { 0 };

    // cm_msg(MINFO,"vmefrontend","V1190 TDC at VME A24 0x%06x:\n", vmeBaseAdress);

    // WORD code = 0x1600;

    code[0] = 0x1600;
    if ((value = V1190WriteOpcode(1, code, handle, vmeBaseAddress)) < 0) {
        std::cerr << "Couldn't read opcode!" << std::endl;
    }

    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    printf("  Match Window width       : 0x%04x, %d ns\n", value, value*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    printf("  Window offset            : 0x%04x, %d ns\n", value, (value - (0xffff + 0x0001))*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    printf("  Extra Search Window Width: 0x%04x, %d ns\n", value, value*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    printf("  Reject Margin            : 0x%04x, %d ns\n", value, value*ns);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    printf("  Trigger Time subtraction : %s\n",(value & 0x1) ? "y" : "n");

    code[0] = 0x2300;
    value = V1190WriteOpcode(1, code, handle, vmeBaseAddress);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    printf("  Edge Detection (0:P/1:T/2:L/3:TL) : 0x%02x\n", (value&0x3));

    unsigned short edge = (value&0x3);

    code[0] = 0x2600;
    value = V1190WriteOpcode(1, code, handle, vmeBaseAddress);
    value = V1190ReadRegister(OPCODE, handle, vmeBaseAddress);
    if (edge == 0x00) {
        printf("  Resolution Edge      : 0x%04x\n", value & 0x0007);
        printf("  Resolution Width     : 0x%04x\n", (value & 0x0F00) >> 8);
    } else {
        printf(" Resolution Edges      : 0x%04x\n", value & 0x0003);
    }

}

void v1190::getStatusReg()
{
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
        printf("\nStrange status register! 0x%04X\n",sr);
        // cm_msg(MERROR, "vmefrontend", "Strange status register! 0x%04X",sr);
    }
}


bool v1190::start()
{

    code[0] = 0x4200;  // enable all channels
    code[1] = 0x1;
    value = V1190WriteOpcode(2, code, handle, vmeBaseAddress);
    V1190SoftClear(0, handle, vmeBaseAddress);

    return true;
}

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