#ifndef TDCEVENT_H
#define TDCEVENT_H

#include <iostream>
#include <vector>
#include <cstdint>
#include <cmath>  // For std::nan
#include <TTree.h>
#include <TFile.h>
#include <TObject.h>
#include <TClass.h>
#include <TROOT.h>

constexpr uint32_t UINT32_UNSET = static_cast<uint32_t>(-1);
constexpr Bool_t BOOL_UNSET = 2;

struct TDCEvent {
    UInt_t eventID;
    Double_t timestamp;
    UInt_t cuspRunNumber;
    Bool_t gate;
    Double_t tdcTimeTag;
    
    std::array<Double_t, 4> trgLE;
    std::array<Double_t, 4> trgTE;

    std::array<Double_t, 32> hodoIDsLE;
    std::array<Double_t, 32> hodoIUsLE;
    std::array<Double_t, 32> hodoODsLE;
    std::array<Double_t, 32> hodoOUsLE;

    std::array<Double_t, 32> hodoIDsTE;
    std::array<Double_t, 32> hodoIUsTE;
    std::array<Double_t, 32> hodoODsTE;
    std::array<Double_t, 32> hodoOUsTE;

    std::array<Double_t, 64> bgoLE;
    std::array<Double_t, 64> bgoTE;

    std::array<Double_t, 120> tileILE;
    std::array<Double_t, 120> tileITE;
    std::array<Double_t, 120> tileOLE;
    std::array<Double_t, 120> tileOTE;

    UInt_t tdcID;

    TDCEvent();
    void reset();
    virtual ~TDCEvent() {}
    ClassDef(TDCEvent, 1);
};



// // ROOT TTree Event Structure
// struct TDCEvent {
//     UInt_t      eventID;

//     Double_t      timestamp;
//     UInt_t      cuspRunNumber;
//     Bool_t      gate;
//     Double_t    tdcTimeTag;
//     Double_t    trgLE[4];
//     Double_t    trgTE[4];

//     Double_t    hodoIDsLE[32];
//     Double_t    hodoIUsLE[32];
//     Double_t    hodoODsLE[32];
//     Double_t    hodoOUsLE[32];
//     Double_t    hodoIDsTE[32];
//     Double_t    hodoIUsTE[32];
//     Double_t    hodoODsTE[32];
//     Double_t    hodoOUsTE[32];
//     // Double_t    hodoIDsToT[32];
//     // Double_t    hodoIUsToT[32];
//     // Double_t    hodoODsToT[32];
//     // Double_t    hodoOUsToT[32];

//     Double_t    bgoLE[64];
//     Double_t    bgoTE[64];
//     // Double_t    bgoToT[64];

//     Double_t    tileILE[120];
//     Double_t    tileITE[120];
//     // Double_t    tileIToT[120];
//     Double_t    tileOLE[120];
//     Double_t    tileOTE[120];
//     // Double_t    tileOToT[120];

//     UInt_t      tdcID;
//     // uint32_t tdcChannel;
//     // uint32_t tdcTime;

//     TDCEvent();  // Constructor declaration
//     void reset(); // Reset function declaration
// };


#endif