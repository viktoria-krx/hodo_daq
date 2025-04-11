#include "fileReader.hh"
#include "logger.hh"
#include <cstring>
#include <sys/stat.h>  // For file size checking


// Constructor: Opens binary file
FileReader::FileReader(const std::string& filename) {
    auto log = Logger::getLogger();
    file.open(filename, std::ios::binary);
    if (!file) {
        log->error("Error opening file: {0}", filename);
    }
}

// Check if file is open
bool FileReader::isOpen() const {
    return file.is_open();
}

long FileReader::getFileSize() {
    struct stat st; 
    if (stat(filename.c_str(), &st) == 0){
        return st.st_size;
    }
        
    return -1;
}

// Read a single DataBank
bool FileReader::readDataBank(DataBank& bank) {
    // Read Bank Name (4 bytes)
    if (!file.read(bank.bankName, 4)) return false;

    // Read Number of Events (4 bytes)
    uint32_t eventCount;
    if (!file.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount))) return false;

    // Read Events
    bank.events.clear();
    for (uint32_t i = 0; i < eventCount; i++) {
        Event event;

        // Read Timestamp (4 bytes)
        if (!file.read(reinterpret_cast<char*>(&event.timestamp), sizeof(event.timestamp))) return false;

        // Read Number of Data Points (4 bytes)
        uint32_t dataSize;
        if (!file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize))) return false;

        // Read Data Points (4 * dataSize bytes)
        event.data.resize(dataSize);
        if (!file.read(reinterpret_cast<char*>(event.data.data()), dataSize * sizeof(uint32_t))) return false;

        bank.events.push_back(event);
    }
    return true;
}

// Read Next Block
bool FileReader::readNextBlock(Block& block, long startPos) {

    file.seekg(startPos, std::ios::beg);
    
    // Read Block ID (4 bytes)
    if (!file.read(reinterpret_cast<char*>(&block.blockID), sizeof(block.blockID))) return false;

    // Read Number of Banks (4 bytes)
    uint32_t bankCount;
    if (!file.read(reinterpret_cast<char*>(&bankCount), sizeof(bankCount))) return false;

    // Read Banks
    block.banks.clear();
    for (uint32_t i = 0; i < bankCount; i++) {
        DataBank bank;
        if (!readDataBank(bank)) return false;
        block.banks.push_back(bank);
    }
    return true;
}