#include "tdcEvent.hh"

// Constructor definition
TDCEvent::TDCEvent() {
    reset();
}

void TDCEvent::reset() {
    // Reset arrays using std::fill
    std::fill(std::begin(trgLE), std::end(trgLE), std::nan(""));
    std::fill(std::begin(trgTE), std::end(trgTE), std::nan(""));

    std::fill(std::begin(hodoIDsLE), std::end(hodoIDsLE), std::nan(""));
    std::fill(std::begin(hodoIUsLE), std::end(hodoIUsLE), std::nan(""));
    std::fill(std::begin(hodoODsLE), std::end(hodoODsLE), std::nan(""));
    std::fill(std::begin(hodoOUsLE), std::end(hodoOUsLE), std::nan(""));
    std::fill(std::begin(hodoIDsTE), std::end(hodoIDsTE), std::nan(""));
    std::fill(std::begin(hodoIUsTE), std::end(hodoIUsTE), std::nan(""));
    std::fill(std::begin(hodoODsTE), std::end(hodoODsTE), std::nan(""));
    std::fill(std::begin(hodoOUsTE), std::end(hodoOUsTE), std::nan(""));
    // std::fill(std::begin(hodoIDsToT), std::end(hodoIDsToT), std::nan(""));
    // std::fill(std::begin(hodoIUsToT), std::end(hodoIUsToT), std::nan(""));
    // std::fill(std::begin(hodoODsToT), std::end(hodoODsToT), std::nan(""));
    // std::fill(std::begin(hodoOUsToT), std::end(hodoOUsToT), std::nan(""));

    std::fill(std::begin(bgoLE), std::end(bgoLE), std::nan(""));
    std::fill(std::begin(bgoTE), std::end(bgoTE), std::nan(""));
    // std::fill(std::begin(bgoToT), std::end(bgoToT), std::nan(""));

    std::fill(std::begin(tileILE), std::end(tileILE), std::nan(""));
    std::fill(std::begin(tileITE), std::end(tileITE), std::nan(""));
    // std::fill(std::begin(tileIToT), std::end(tileIToT), std::nan(""));
    std::fill(std::begin(tileOLE), std::end(tileOLE), std::nan(""));
    std::fill(std::begin(tileOTE), std::end(tileOTE), std::nan(""));
    // std::fill(std::begin(tileOToT), std::end(tileOToT), std::nan(""));

    // Reset individual scalar variables
    eventID = UINT32_UNSET;
    timestamp = std::nan("");
    cuspRunNumber = UINT32_UNSET;
    gate = BOOL_UNSET;
    tdcTimeTag = std::nan("");
    tdcID = UINT32_UNSET;
}

ClassImp(TDCEvent);  // This implements the ROOT type system for TDCEvent