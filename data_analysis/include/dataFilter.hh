#ifndef DATAFILTER_H
#define DATAFILTER_H

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <string>
#include <cmath>    
#include <cstddef>  
#include "dataDecoder.hh"
#include "logger.hh"

class DataFilter {
public:
    DataFilter(const char* inputFile);
    ~DataFilter();
    void filterAndSend(const char* inputFile, int last_evt);
    void fileSorter(const char* inputFile, int last_evt, const char* outputFileName);
private:
    int LE_CUT = 400;   // ns
    int ToT_CUT = 200;  // ns 
};

#endif