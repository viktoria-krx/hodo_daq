#include "dataBanks.hh"
#include <cstring>  // for memcpy, strncpy
#include <iostream> // for debugging

DataBank::DataBank() {}

DataBank::~DataBank() {}

DataBank::DataBank(const char* name) {
    strncpy(bankName, name, 4); // Copy exactly 4 characters
}

/**
 * @brief Adds an event to the data bank.
 *
 * @param event The event to be added to the data bank.
 */

void DataBank::addEvent(const Event& event) {
    events.push_back(event);
}

/**
 * @brief Serialize the data bank for writing.
 *
 * The format of the serialized data bank is as follows:
 * 1. The 4-character bank name (ASCII)
 * 2. The number of events in the bank (uint32_t)
 * 3. For each event:
 *    a. The timestamp of the event (uint32_t)
 *    b. The number of data points in the event (uint32_t)
 *    c. The data points themselves (uint32_t array)
 *
 * @return The serialized data bank as a vector of bytes.
 */
std::vector<uint32_t> DataBank::serialize() const {
    std::vector<uint32_t> buffer;

    // Write Bank Name (4 bytes)
    uint32_t packedBankName = (bankName[3] << 24) | (bankName[2] << 16) | (bankName[1] << 8) | (bankName[0]);
    // buffer.insert(buffer.end(), bankName, bankName + 4);
    buffer.push_back(packedBankName);

    // Write number of events
    uint32_t eventCount = events.size();
    // buffer.insert(buffer.end(), reinterpret_cast<const uint32_t*>(&eventCount), 
    //               reinterpret_cast<const uint32_t*>(&eventCount) + sizeof(eventCount));
    buffer.push_back(eventCount);

    // Serialize each event
    for (const auto& event : events) {
        // buffer.insert(buffer.end(), reinterpret_cast<const uint32_t*>(&event.timestamp),
        //               reinterpret_cast<const uint32_t*>(&event.timestamp) + sizeof(event.timestamp));

        // adding this for 64 bit time stamps:
        if (event.timestamp64 > 0) {
            uint32_t ts_low  = static_cast<uint32_t>(event.timestamp64 & 0xFFFFFFFF);
            uint32_t ts_high = static_cast<uint32_t>((event.timestamp64 >> 32) & 0xFFFFFFFF);
            buffer.push_back(ts_high);
            buffer.push_back(ts_low);
        }
        else if (event.timestamp > 0) {
            buffer.push_back(event.timestamp);       
        }
        

        uint32_t dataSize = event.data.size();
        // buffer.insert(buffer.end(), reinterpret_cast<const uint32_t*>(&dataSize),
        //               reinterpret_cast<const uint32_t*>(&dataSize) + sizeof(dataSize));
        buffer.push_back(dataSize);

        if (!event.data.empty()) {
            // buffer.insert(buffer.end(), reinterpret_cast<const uint32_t*>(event.data.data()),
            //               reinterpret_cast<const uint32_t*>(event.data.data()) + dataSize * sizeof(uint32_t));
            buffer.insert(buffer.end(), event.data.begin(), event.data.end());
        }
    }

    return buffer;
}

void DataBank::clear() {
    events.clear();
}

const char* DataBank::getBankName() const {
    return bankName;
}

const std::vector<Event>& DataBank::getEvents() const {
    return events;
}


// ----- Blocks ----- 

Block::Block(uint32_t id) : blockID(id) { }

Block::~Block() {}

void Block::addDataBank(const DataBank& bank) {
    banks.push_back(bank);
}

/**
 * @brief Serialize the block into a binary vector.
 * 
 * This function returns a binary vector that contains all the data in the block.
 * The vector is ordered as follows:
 * - Block ID (4 bytes)
 * - Number of DataBanks (4 bytes)
 * - Serialized DataBanks (order is not guaranteed)
 * 
 * @return A binary vector containing the serialized block data.
 */
std::vector<uint32_t> Block::serialize() const {
    std::vector<uint32_t> buffer;

    // Write Block ID (4 bytes)
    buffer.push_back(blockID);
    // buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&blockID), 
    //               reinterpret_cast<const uint8_t*>(&blockID) + sizeof(blockID));

    // Write number of banks (4 bytes)
    uint32_t bankCount = banks.size();
    buffer.push_back(bankCount);
    // buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&bankCount),
    //               reinterpret_cast<const uint8_t*>(&bankCount) + sizeof(bankCount));

    // Serialize each bank
    for (const auto& bank : banks) {
        std::vector<uint32_t> bankData = bank.serialize();
        buffer.insert(buffer.end(), bankData.begin(), bankData.end());
    }

    return buffer;
}

void Block::clear() {
    banks.clear();
}

const std::vector<DataBank>& Block::getDataBanks() const {
    return banks;
}
