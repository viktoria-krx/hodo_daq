#ifndef DATAFILTER_H
#define DATAFILTER_H

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <string>
#include <cmath>    
#include <cstddef>  
#include <zmq.hpp>
#include "dataDecoder.hh"
#include "logger.hh"
#include "tdcEvent.hh"

class DataFilter {
public:
    DataFilter(){};
    ~DataFilter(){};
    void filterAndSend(const char* inputFile, int last_evt, zmq::socket_t& socket);
    void filterAndSaveAndSend(const char* inputFile, int last_evt, zmq::socket_t& socket);
    void filterAndSave(const char* inputFile, int last_evt);
    void fileSorter(const char* inputFile, int last_evt, const char* outputFileName);
    void convertTime(TDCEvent& event);
private:
    const Double_t LE_CUT = 400.;   // ns
    const Double_t ToT_CUT = 200.;  // ns 
};

#endif