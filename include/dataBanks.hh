#ifndef DATA_BANK_HH
#define DATA_BANK_HH

#include "eventStruct.hh"
#include <vector>
#include <cstdint>

class DataBank {
public:
    DataBank(const char* bankName);

    void addEvent(const Event& event);
    std::vector<uint8_t> serialize() const;
    void clear();

    const char* getBankName() const;
    const std::vector<Event>& getEvents() const;

private:
    // uint32_t bankID;
    char bankName[4];
    std::vector<Event> events;
};

#endif // DATA_BANK_HH