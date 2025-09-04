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
    auto log = Logger::getLogger();
    uint32_t packedBankName;
    // Read Bank Name (4 bytes)
    // if (!file.read(bank.bankName, 4)) return false;
    if (!file.read(reinterpret_cast<char*>(&packedBankName), sizeof(packedBankName))) return false;

    // Unpack the bank name from the 32-bit value
    bank.bankName[0] =  packedBankName & 0xFF;           // First byte (least significant byte)
    bank.bankName[1] = (packedBankName >> 8) & 0xFF;    // Second byte
    bank.bankName[2] = (packedBankName >> 16) & 0xFF;   // Third byte
    bank.bankName[3] = (packedBankName >> 24) & 0xFF;   // Fourth byte (most significant byte)

    std::string bankNameStr(bank.bankName, 4);

    // Read Number of Events (4 bytes)
    uint32_t eventCount;
    if (!file.read(reinterpret_cast<char*>(&eventCount), sizeof(eventCount))) return false;

    // Read Events
    bank.events.clear();
    for (uint32_t i = 0; i < eventCount; i++) {
        Event event;

        if (bankNameStr == "CUSP") {
            
            uint32_t tsHigh;
            if (!file.read(reinterpret_cast<char*>(&tsHigh), sizeof(tsHigh))) return false;

            if (tsHigh & 0x80000000) {  
                // --- New format, 64-bit ---
                uint32_t tsLow;
                if (!file.read(reinterpret_cast<char*>(&tsLow), sizeof(tsLow))) return false;
                
                uint64_t ts64 = (uint64_t(tsHigh & 0x7FFFFFFF) << 32) | tsLow;
                event.timestamp64 = ts64;
                event.timestamp = 0;
                flag64 = true;
            } else {
                // --- Old format, 32-bit ---
                event.timestamp = tsHigh;        // this was the only word stored
                event.timestamp64 = 0;      // promote for consistency
                // flag64 = false;
            }
        }
        else if (bankNameStr == "GATE" && flag64) {
            // --- GATE in 64-bit mode ---
            uint32_t tsHigh, tsLow;
            if (!file.read(reinterpret_cast<char*>(&tsHigh), sizeof(tsHigh))) return false;
            if (!file.read(reinterpret_cast<char*>(&tsLow), sizeof(tsLow))) return false;

            uint64_t ts64 = (uint64_t(tsHigh) << 32) | tsLow;
            // log->debug("fpgaTimeTag: {}", ts64);
            event.timestamp64 = ts64;
            event.timestamp = 0;
        }  
        else {
            // --- All other banks: 32-bit timestamp ---
            if (!file.read(reinterpret_cast<char*>(&event.timestamp), sizeof(event.timestamp))) return false;
            event.timestamp64 = 0;  
        }

        // // Read Timestamp (4 bytes)
        // if (!file.read(reinterpret_cast<char*>(&event.timestamp), sizeof(event.timestamp))) return false;

        // Read Number of Data Points (4 bytes)
        uint32_t dataSize;
        if (!file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize))) return false;

        if (dataSize > 10000) { 
            log->error("Unrealistic dataSize {} in bank {} at offset 0x{:x}", dataSize, bankNameStr, static_cast<long long>(file.tellg()));

            std::string resyncedBank;
            if (resyncToNextBank(file, resyncedBank)) {
                log->warn("Resynced to bank {} at offset 0x{:x}", 
                        resyncedBank, static_cast<long long>(file.tellg()));
                // You’d now return control so the next `readDataBank()` starts at the new bank
                return true;
            } else {
                log->error("Failed to resync, reached EOF.");
                return false;
            }
        }

        // Read Data Points (4 * dataSize bytes)
        event.data.resize(dataSize);
        if (!file.read(reinterpret_cast<char*>(event.data.data()), dataSize * sizeof(uint32_t))) return false;

        bank.events.push_back(std::move(event));
    }
    return true;
}

bool FileReader::resyncToNextBank(std::ifstream& file, std::string& bankNameOut) {
    auto log = Logger::getLogger();
    uint32_t candidate;
    while (file.read(reinterpret_cast<char*>(&candidate), sizeof(candidate))) {
        char name[5];
        name[0] =  candidate        & 0xFF;
        name[1] = (candidate >> 8)  & 0xFF;
        name[2] = (candidate >> 16) & 0xFF;
        name[3] = (candidate >> 24) & 0xFF;
        name[4] = '\0';

        std::string bankName(name);

        if (bankName == "CUSP" || bankName == "GATE" || bankName.rfind("TDC", 0) == 0) {
            bankNameOut = bankName;
            log->debug("Found next bank: {} at offset =x{:x}", bankName, static_cast<long long>(file.tellg()));
            // move back 4 bytes so the caller reads the bank name itself
            file.seekg(-4, std::ios::cur);
            return true;  // found valid bank
        }
    }
    return false; // EOF reached without finding new bank
}


// Read Next Block
bool FileReader::readNextBlock(Block& block, long startPos) {
    auto log = Logger::getLogger();
    // log->debug("readNextBlock");
    if (!file.is_open()) {
        std::cerr << "Failed to open file in readNextBlock.\n";
        return false;
    }

    file.clear(); // reset state in case of previous EOF
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

    currentPos = static_cast<long>(file.tellg());

    return true;
}