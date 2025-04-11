#ifndef EVENT_HH
#define EVENT_HH

#include <vector>
#include <cstdint>

struct Event {
    uint32_t eventID;
    uint64_t timestamp;
    std::vector<uint32_t> data; // Raw TDC hits, for example
};

#endif // EVENT_HH