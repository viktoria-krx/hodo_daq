#ifndef DATADECODER_H
#define DATADECODER_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>  // For std::nan
#include <TTree.h>
#include <TFile.h>
#include "fileReader.hh"


// ROOT TTree Event Structure
struct TDCEvent {
    UInt_t      eventID;

    Double_t      timestamp;
    UInt_t      cuspRunNumber;
    Bool_t      gate;
    Double_t    tdcTimeTag;
    Double_t    trgLE[4];
    Double_t    trgTE[4];

    Double_t    hodoIDsLE[32];
    Double_t    hodoIUsLE[32];
    Double_t    hodoODsLE[32];
    Double_t    hodoOUsLE[32];
    Double_t    hodoIDsTE[32];
    Double_t    hodoIUsTE[32];
    Double_t    hodoODsTE[32];
    Double_t    hodoOUsTE[32];
    // Double_t    hodoIDsToT[32];
    // Double_t    hodoIUsToT[32];
    // Double_t    hodoODsToT[32];
    // Double_t    hodoOUsToT[32];

    Double_t    bgoLE[64];
    Double_t    bgoTE[64];
    // Double_t    bgoToT[64];

    Double_t    tileILE[120];
    Double_t    tileITE[120];
    // Double_t    tileIToT[120];
    Double_t    tileOLE[120];
    Double_t    tileOTE[120];
    // Double_t    tileOToT[120];

    UInt_t      tdcID;
    // uint32_t tdcChannel;
    // uint32_t tdcTime;

    TDCEvent();  // Constructor declaration
    void reset(); // Reset function declaration
};




// DataDecoder Class
class DataDecoder {
public:
    DataDecoder(const std::string& outputFile);
    ~DataDecoder();
    
    void processBlock(const std::vector<uint32_t>& rawData);  // Decode and store data
    void writeTree();  // Write to ROOT file
    bool fillData(int channel, int rawchannel, int edge, int32_t time, TDCEvent &event);
    void processEvent(const char bankName[4], Event dataevent);
    constexpr int getChannel(int ch);
    void flush();

private:
    TFile* rootFile;
    TTree* tree;
    TDCEvent event;
    int32_t ch, rawch;
    int le_te;
    int tdcID;
    uint32_t geo = 0;
    uint64_t timetag = 0;
    int32_t data_time;
    int32_t gateTime;
    bool gateValue = 0;
    int32_t cuspValue = 0;
    uint32_t secTime = 0;
    int32_t this_evt[4] = {0};
    int32_t last_evt[4] = {0};
    int32_t reset_ctr[4] = {0};

};



#endif