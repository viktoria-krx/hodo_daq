#ifndef DATA_BANK_HH
#define DATA_BANK_HH

#include "eventStruct.hh"
#include <vector>
#include <cstdint>

class DataBank {
public:
    DataBank();
    ~DataBank();
    DataBank(const char* bankName);

    void addEvent(const Event& event);
    std::vector<uint32_t> serialize() const;
    void clear();

    const char* getBankName() const;
    const std::vector<Event>& getEvents() const;

    char bankName[4];

private:
    // uint32_t bankID;
    
    std::vector<Event> events;
};

class Block {
public:
    Block(uint32_t id);
    ~Block();
    void addDataBank(const DataBank& bank);
    std::vector<uint32_t> serialize() const;
    void clear();
    const std::vector<DataBank>& getDataBanks() const;
private:
    uint32_t blockID;
    std::vector<DataBank> banks;
};

#endif // DATA_BANK_HH