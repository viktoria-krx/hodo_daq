#include "dataBanks.hh"
#include <cstring>  // for memcpy, strncpy
#include <iostream> // for debugging

DataBank::DataBank() {}

DataBank::~DataBank() {}

DataBank::DataBank(const char* name) {
    strncpy(bankName, name, 4); // Copy exactly 4 characters
}

void DataBank::addEvent(const Event& event) {
    events.push_back(event);
}

std::vector<uint8_t> DataBank::serialize() const {
    std::vector<uint8_t> buffer;

    // Write Bank Name (4 bytes)
    buffer.insert(buffer.end(), bankName, bankName + 4);

    // Write number of events
    uint32_t eventCount = events.size();
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&eventCount), 
                  reinterpret_cast<const uint8_t*>(&eventCount) + sizeof(eventCount));

    // Serialize each event
    for (const auto& event : events) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&event.timestamp),
                      reinterpret_cast<const uint8_t*>(&event.timestamp) + sizeof(event.timestamp));

        uint32_t dataSize = event.data.size();
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&dataSize),
                      reinterpret_cast<const uint8_t*>(&dataSize) + sizeof(dataSize));

        if (!event.data.empty()) {
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(event.data.data()),
                          reinterpret_cast<const uint8_t*>(event.data.data()) + dataSize * sizeof(uint16_t));
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

void Block::addDataBank(const DataBank& bank) {
    banks.push_back(bank);
}

std::vector<uint8_t> Block::serialize() const {
    std::vector<uint8_t> buffer;

    // Write Block ID (4 bytes)
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&blockID), 
                  reinterpret_cast<const uint8_t*>(&blockID) + sizeof(blockID));

    // Write number of banks (4 bytes)
    uint32_t bankCount = banks.size();
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&bankCount),
                  reinterpret_cast<const uint8_t*>(&bankCount) + sizeof(bankCount));

    // Serialize each bank
    for (const auto& bank : banks) {
        std::vector<uint8_t> bankData = bank.serialize();
        buffer.insert(buffer.end(), bankData.begin(), bankData.end());
    }

    return buffer;
}

void Block::clear() {
    banks.clear();
}

const std::vector<DataBank>& Block ::getDataBanks() const {
    return banks;
}
