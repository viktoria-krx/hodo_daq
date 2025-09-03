#ifndef EVENT_HH
#define EVENT_HH

#include <vector>
#include <cstdint>

struct Event {
    uint32_t eventID;
    uint32_t timestamp;
    uint64_t timestamp64;
    std::vector<uint32_t> data; // Raw TDC hits, for example
};

#endif // EVENT_HH