#ifndef DATADECODER_H
#define DATADECODER_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>  // For std::nan
#include <TTree.h>
#include <TFile.h>
#include <TObject.h>
#include <TClass.h>
#include <TROOT.h>

#include "fileReader.hh"
#include "tdcEvent.hh"


// DataDecoder Class
class DataDecoder {
public:
    DataDecoder(const std::string& outputFile);
    ~DataDecoder();
    
    void processBlock(const std::vector<uint32_t>& rawData);  // Decode and store data
    void writeTree();  // Write to ROOT file
    bool fillData(int channel, int rawchannel, int edge, int32_t time, TDCEvent &event);
    uint32_t processEvent(const char bankName[4], Event dataevent);
    constexpr int getChannel(int ch);
    void flush();
    const char* getFileName() { return fileName.c_str(); }

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
    Double_t secTime = 0;
    int32_t this_evt[4] = {0};
    int32_t last_evt[4] = {0};
    int32_t reset_ctr[4] = {0};
    Double_t this_timetag[4] = {0.};
    Double_t last_timetag[4] = {0.};
    int32_t reset_ctr_time[4] = {0};
    std::string fileName;

};



#endif