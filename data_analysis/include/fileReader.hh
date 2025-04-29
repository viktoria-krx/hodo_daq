#ifndef FILEREADER_H
#define FILEREADER_H

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <string>


#define DATA_TYPE(r)           (((r)>>27)  & 0x1F)
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
#define DATA_EDGE(r)           (((r)      & 0x04000000) >>26)
#define EXTENDED_TT(r)         (((r)      & 0x7FFFFFF))
#define ETTT_GEO(r)            (((r)      & 0x0000001F))

// Event Structure
struct Event {
    uint32_t timestamp;
    std::vector<uint32_t> data;
};

// DataBank Structure
struct DataBank {
    char bankName[4];
    std::vector<Event> events;
};

// Block Structure
struct Block {
    uint32_t blockID;
    std::vector<DataBank> banks;
};

// File Reader Class
class FileReader {
public:
    FileReader(const std::string& filename);
    bool readNextBlock(Block& block, long startPos);  // Read the next block of data
    bool isOpen() const;
    long getFileSize();
    long currentPos;

private:
    std::ifstream file;
    bool readDataBank(DataBank& bank);
    const std::string& filename = "";
    
};

#endif