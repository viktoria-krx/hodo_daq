#include "dataBanks.hh"
#include <cstring>  // for memcpy, strncpy
#include <iostream> // for debugging

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