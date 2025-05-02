#include <algorithm> // For std::fill_n
#include <cmath>     // For std::nan

#include "dataDecoder.hh"
#include "fileReader.hh"
#include "logger.hh"



// Constructor: Initializes ROOT File & TTree
DataDecoder::DataDecoder(const std::string& outputFile) {
    rootFile = new TFile(outputFile.c_str(), "RECREATE");
    tree = new TTree("RawEventTree", "TTree holding the raw Hodoscope Data");

    // Define Tree Branches
    tree->Branch("eventID", &event.eventID);
    tree->Branch("timestamp", &event.timestamp);
    tree->Branch("cuspRunNumber", &event.cuspRunNumber);
    tree->Branch("gate", &event.gate);
    tree->Branch("tdcTimeTag", &event.tdcTimeTag);
    tree->Branch("trgLE", &event.trgLE);
    tree->Branch("trgTE", &event.trgTE);

    tree->Branch("hodoIDsLE", &event.hodoIDsLE);     // Inner Downstream Leading Edges
    tree->Branch("hodoIUsLE", &event.hodoIUsLE);     // Inner Upstream Leading Edges
    tree->Branch("hodoODsLE", &event.hodoODsLE);     // Outer Downstream Leading Edges
    tree->Branch("hodoOUsLE", &event.hodoOUsLE);     // Outer Upstream Leading Edges
    tree->Branch("hodoIDsTE", &event.hodoIDsTE);     // Inner Downstream Trailing Edges
    tree->Branch("hodoIUsTE", &event.hodoIUsTE);     // Inner Upstream Trailing Edges
    tree->Branch("hodoODsTE", &event.hodoODsTE);     // Outer Downstream Trailing Edges
    tree->Branch("hodoOUsTE", &event.hodoOUsTE);     // Outer Upstream Trailing Edges
    // tree->Branch("hodoIDsToT", &event.hodoIDsToT, "hodoIDsToT[32]/D");     // Inner Downstream Time over Threshold
    // tree->Branch("hodoIUsToT", &event.hodoIUsToT, "hodoIUsTEoT[32]/D");     // Inner Upstream Time over Threshold
    // tree->Branch("hodoODsToT", &event.hodoODsToT, "hodoODsToT[32]/D");     // Outer Downstream Time over Threshold
    // tree->Branch("hodoOUsToT", &event.hodoOUsToT, "hodoOUsToT[32]/D");     // Outer Upstream Time over Threshold

    tree->Branch("bgoLE", &event.bgoLE);     // BGO Leading Edges
    tree->Branch("bgoTE", &event.bgoTE);     // BGO Trailing Edges
    // tree->Branch("bgoToT", &event.bgoToT, "bgoToT[64]/D");  // BGO Time over Threshold

    tree->Branch("tileILE", &event.tileILE);     // Tile Inner Leading Edges
    tree->Branch("tileITE", &event.tileITE);     // Tile Inner Trailing Edges
    tree->Branch("tileOLE", &event.tileOLE);     // Tile Outer Leading Edges
    tree->Branch("tileOTE", &event.tileOTE);     // Tile Outer Trailing Edges
    // tree->Branch("tileIToT", &event.tileIToT, "tileIToT[120]/D");     // Tile Inner Time over Threshold
    // tree->Branch("tileOToT", &event.tileOToT, "tileOToT[120]/D");     // Tile Outer Time over Threshold

    tree->Branch("tdcID", &event.tdcID);
    // tree->Branch("tdcChannel", &event.tdcChannel, "tdcChannel/i");
    
}
// // Constructor: Initializes ROOT File & TTree
// DataDecoder::DataDecoder(const std::string& outputFile) {
//     rootFile = new TFile(outputFile.c_str(), "RECREATE");
//     tree = new TTree("RawEventTree", "TTree holding the raw Hodoscope Data");

//     // Define Tree Branches
//     tree->Branch("eventID", &event.eventID, "eventID/i");
//     tree->Branch("timestamp", &event.timestamp, "timestamp/D");
//     tree->Branch("cuspRunNumber", &event.cuspRunNumber, "cuspRunNumber/i");
//     tree->Branch("gate", &event.gate, "gate/O");
//     tree->Branch("tdcTimeTag", &event.tdcTimeTag, "tdcTimeTag/D");
//     tree->Branch("trgLE", &event.trgLE, "trgLE[4]/D");
//     tree->Branch("trgTE", &event.trgTE, "trgTE[4]/D");

//     tree->Branch("hodoIDsLE", &event.hodoIDsLE, "hodoIDsLE[32]/D");     // Inner Downstream Leading Edges
//     tree->Branch("hodoIUsLE", &event.hodoIUsLE, "hodoIUsLE[32]/D");     // Inner Upstream Leading Edges
//     tree->Branch("hodoODsLE", &event.hodoODsLE, "hodoODsLE[32]/D");     // Outer Downstream Leading Edges
//     tree->Branch("hodoOUsLE", &event.hodoOUsLE, "hodoOUsLE[32]/D");     // Outer Upstream Leading Edges
//     tree->Branch("hodoIDsTE", &event.hodoIDsTE, "hodoIDsTE[32]/D");     // Inner Downstream Trailing Edges
//     tree->Branch("hodoIUsTE", &event.hodoIUsTE, "hodoIUsTE[32]/D");     // Inner Upstream Trailing Edges
//     tree->Branch("hodoODsTE", &event.hodoODsTE, "hodoODsTE[32]/D");     // Outer Downstream Trailing Edges
//     tree->Branch("hodoOUsTE", &event.hodoOUsTE, "hodoOUsTE[32]/D");     // Outer Upstream Trailing Edges
//     // tree->Branch("hodoIDsToT", &event.hodoIDsToT, "hodoIDsToT[32]/D");     // Inner Downstream Time over Threshold
//     // tree->Branch("hodoIUsToT", &event.hodoIUsToT, "hodoIUsTEoT[32]/D");     // Inner Upstream Time over Threshold
//     // tree->Branch("hodoODsToT", &event.hodoODsToT, "hodoODsToT[32]/D");     // Outer Downstream Time over Threshold
//     // tree->Branch("hodoOUsToT", &event.hodoOUsToT, "hodoOUsToT[32]/D");     // Outer Upstream Time over Threshold

//     tree->Branch("bgoLE", &event.bgoLE, "bgoLE[64]/D");     // BGO Leading Edges
//     tree->Branch("bgoTE", &event.bgoTE, "bgoTE[64]/D");     // BGO Trailing Edges
//     // tree->Branch("bgoToT", &event.bgoToT, "bgoToT[64]/D");  // BGO Time over Threshold

//     tree->Branch("tileILE", &event.tileILE, "tileILE[120]/D");     // Tile Inner Leading Edges
//     tree->Branch("tileITE", &event.tileITE, "tileITE[120]/D");     // Tile Inner Trailing Edges
//     tree->Branch("tileOLE", &event.tileOLE, "tileOLE[120]/D");     // Tile Outer Leading Edges
//     tree->Branch("tileOTE", &event.tileOTE, "tileOTE[120]/D");     // Tile Outer Trailing Edges
//     // tree->Branch("tileIToT", &event.tileIToT, "tileIToT[120]/D");     // Tile Inner Time over Threshold
//     // tree->Branch("tileOToT", &event.tileOToT, "tileOToT[120]/D");     // Tile Outer Time over Threshold

//     tree->Branch("tdcID", &event.tdcID, "tdcID/i");
//     // tree->Branch("tdcChannel", &event.tdcChannel, "tdcChannel/i");
    
// }



// Destructor: Writes and Closes ROOT File
DataDecoder::~DataDecoder() {
    writeTree();
    rootFile->Close();
    delete rootFile;
}


// Lookup table for detector types
enum DetectorType {
    HODO_IDs, 
    HODO_IUs, 
    TILE_IT, 
    HODO_ODs, 
    HODO_OUs, 
    TILE_OT, 
    BGO, 
    TRG, 
    GATE,
    UNKNOWN
};

std::map<int, DetectorType> channelMap = {
    { -90,   TILE_IT   },
    { -50,   HODO_IUs  },
    {  -1,   HODO_IDs  },
    {   1,   HODO_ODs  }, 
    {  50,   HODO_OUs  }, 
    { 100,   TILE_OT   }, 
    { 300,   BGO       }, 
    { 500,   TRG       }, 
    { 600,   UNKNOWN   }  
};


// Function to classify channels
// DetectorType getDetectorType(int channel) {
//     for (const auto& [threshold, type] : channelMap) {
//         if (channel >= threshold) {
//             return type;
//         }
//     }
//     return UNKNOWN;
// }

DetectorType getDetectorType(int channel) {
    if (channel < 0) {
        if (channel > -50) return HODO_IDs;         // Inner Downstream Bars
        else if (channel > -90) return HODO_IUs;    // Inner Upstream Bars
        else return TILE_IT;                        // Inner Tiles
    } else {
        if (channel >= 1 && channel < 50) return HODO_ODs;          // Outer Downstream Bars
        else if (channel >= 50 && channel < 90) return HODO_OUs;    // Outer Upstream Bars
        else if (channel >= 100 && channel < 250) return TILE_OT;   // Outer Tiles
        else if (channel >= 300 && channel < 400) return BGO;       // BGO
        else if (channel >= 500 && channel < 600) return TRG;       // Trigger
        else if (channel == 600) return GATE;                       // Gate
        return UNKNOWN;                                             // For any unknown channels
    }
}


// Process time assignment dynamically
void assignTime(TDCEvent &event, int channel, int edge, int32_t time, DetectorType type) {
    Double_t* storage = nullptr;

    // Select correct map based on detector type
    switch (type) {
        case HODO_IDs: storage = (edge == 0) ? &event.hodoIDsTE[channel] : &event.hodoIDsLE[channel]; break;
        case HODO_IUs: storage = (edge == 0) ? &event.hodoIUsTE[channel] : &event.hodoIUsLE[channel]; break;
        case TILE_IT:  storage = (edge == 0) ? &event.tileITE[channel]  : &event.tileILE[channel]; break;
        case HODO_ODs: storage = (edge == 0) ? &event.hodoODsTE[channel] : &event.hodoODsLE[channel]; break;
        case HODO_OUs: storage = (edge == 0) ? &event.hodoOUsTE[channel] : &event.hodoOUsLE[channel]; break;
        case TILE_OT:  storage = (edge == 0) ? &event.tileOTE[channel]  : &event.tileOLE[channel]; break;
        case BGO:      storage = (edge == 0) ? &event.bgoTE[channel]    : &event.bgoLE[channel]; break;
        case TRG:      storage = (edge == 0) ? &event.trgTE[channel]    : &event.trgLE[channel]; break;
        case UNKNOWN: 
            if (channel == 600) { event.gate = true; }
            return;
    }

    if (!storage) return;

    // Assign time only if it's the first entry (`NaN`) or a smaller value
    if (std::isnan(storage[channel]) || time < storage[channel]) {
        *storage = time;
    }
}


// Optimized fillData function
bool DataDecoder::fillData(int channel, int rawchannel, int edge, int32_t time, TDCEvent &event) {
    auto log = Logger::getLogger();

    DetectorType type = getDetectorType(channel);

    if (channel == 32 || channel == -32) { channel = 0; }  // Normalize channel 32

    // Convert negative channels
    if (channel < 0) {
        if      (channel > -50) channel *= -1;           // Inner Downstream Bars
        else if (channel > -90) channel  = (-channel) - 50;  // Inner Upstream Bars
        else     channel = (-channel) - 100;            // Inner Tiles
    } else {
        if      (channel >=  50 && channel <  90) channel -= 50;  // Outer Upstream Bars
        else if (channel >= 100 && channel < 250) channel -= 100; // Outer Tiles
        else if (channel >= 300 && channel < 400) channel -= 300; // BGO
        else if (channel >= 500 && channel < 600) channel -= 500; // Trigger
    }


    // log->debug("Channel: {0:d} | Edge: {1:d} | Time: {2:d} | Type: {3:d}", channel, edge, time, (int)type);
    assignTime(event, channel, edge, time, type);
    return true;
}


void DataDecoder::processEvent(const char bankName[4], Event dataevent) {

    auto log = Logger::getLogger();

    const std::vector<uint32_t>& data = dataevent.data;

    std::string bankN(bankName, 4);

    log->debug("Processing Bank: {0} | Data Size: {1:d}", bankN, data.size());

    if (bankN.find("TDC") == 0) {
        tdcID = bankN.back() - '0';
        for (size_t i = 0; i < data.size(); i++) {
            uint32_t word = data[i];

            if (IS_TDC_DATA(word)) {

                rawch = DATA_CH(word);
                le_te = DATA_EDGE(word);

                ch = rawch + tdcID * 128;
                rawch = ch;
                ch = getChannel(ch);

                data_time = DATA_MEAS(word);

                fillData(ch, rawch, le_te, data_time, event);

                event.tdcID = tdcID;

                // log->debug("[TDC Hit] Bank: {0} | Raw Ch: {1:d} | TDC ID: {2} | Ch: {3:d} | Time: {4:d}", bankN, rawch, tdcID, ch, data_time);
                
            } else if (IS_TDC_HEADER(word)) {
                // log->debug("[TDC Header] Bank: {}", bankN);
            } else if (IS_TRIGGER_TIME_TAG(word)) {

                timetag = DATA_MEAS(word);
                // log->debug("[TDC ETTT] Bank: {}", bankN);

            } else if (IS_TDC_TRAILER(word)) {

                this_evt[tdcID] = DATA_EVENT_ID(word) + reset_ctr[tdcID] * 0x1000 ;
                if (this_evt[tdcID] < last_evt[tdcID]) {
                    reset_ctr[tdcID]++;
                }
                event.eventID = DATA_EVENT_ID(word) + reset_ctr[tdcID] * 0x1000 ;
                last_evt[tdcID] = event.eventID;
                // log->debug("[TDC Trailer] Bank: {} | Event ID: {}", bankN, event.eventID);
            } else if (IS_GLOBAL_HEADER(word)) {
                // log->debug("[Global Header]");
            } else if (IS_GLOBAL_TRAILER(word)) {

                geo = ETTT_GEO(word);
                // log->debug("[Global Trailer] | GEO: {} | TimeTag: {:d}", geo, timetag*32+geo);
                
                event.tdcTimeTag = timetag*32+geo;
                event.cuspRunNumber = cuspValue;
                event.timestamp = static_cast<Double_t>(secTime);
                event.gate = gateValue; 
                tree->Fill();
                event.reset();

            } else {
                log->warn("[Unknown Data] Word: {0:b}", word);
            }
        }
    } else if (bankN == "GATE") {
        if (!data.empty()) {
            gateTime = data[0];
            log->debug("[GATE Event] Time: {0:d}", gateTime);
            //tree->Fill();
            //event.reset();
        }
    } else if (bankN == "CUSP") {
        if (!data.empty()) {
            for (size_t i = 0; i < data.size(); i++) {
            }
            cuspValue = data[0];
            event.cuspRunNumber = cuspValue;
            
            secTime = dataevent.timestamp;
            event.timestamp = secTime;
            // log->debug("[CUSP Event] Value: {0:d} | Time: {1:d}", cuspValue, secTime);
            // tree->Fill();
            // event.reset();
        }
    } else {
        log->warn("Unknown Bank: {0}", bankN);
    }

}

// Write TTree to ROOT File
void DataDecoder::writeTree() {
    rootFile->Write();
    //std::cout << "ROOT file saved.\n";
}



constexpr int DataDecoder::getChannel(int tdcch) {
    // Ensure tdcch is within a valid range
    if (tdcch < 0 || tdcch >= 28 * 16) {
        return -999; // Error value (invalid channel)
    }

    constexpr std::array<std::array<int, 16>, 28> board = {{
        {{   4 ,   5 ,   6 ,   7 ,   8 ,   9 ,  10 ,  11 ,  -4 ,  -5 ,  -6 ,  -7 ,  -8 ,  -9 , -10 , -11 }}, // A0 Bars 4-11
        {{  12 ,  13 ,  14 ,  15 ,  16 ,  17 ,  18 ,  19 , -12 , -13 , -14 , -15 , -16 , -17 , -18 , -19 }}, // A1 Bars 12-19
        {{ -61 , -60 , -59 , -58 , -57 , -56 , -55 , -54 ,  61 ,  60 ,  59 ,  58 ,  57 ,  56 ,  55 ,  54 }}, // D1 Bars 4-11
        {{ -69 , -68 , -67 , -66 , -65 , -64 , -63 , -62 ,  69 ,  68 ,  67 ,  66 ,  65 ,  64 ,  63 ,  62 }}, // D0 Bars 12-19
        {{ 100 , 101 , 102 , 103 , 104 , 105 , 106 , 107 , 115 , 116 , 117 , 118 , 119 , 120 , 121 , 122 }}, // Outer Tiles AmpBoard 10
        {{ 130 , 131 , 132 , 133 , 134 , 135 , 136 , 137 , 145 , 146 , 147 , 148 , 149 , 150 , 151 , 152 }}, // Outer Tiles AmpBoard 11
        {{ 108 , 109 , 110 , 111 , 112 ,-999 , 114 ,-999 , 123 , 124 , 125 , 126 , 127 , 128 , 129 , 113 }}, // Outer Tiles AmpBoard 14
        {{ 138 , 139 , 140 , 141 , 142 , 143 , 144 ,-999 , 153 , 154 , 155 , 156 , 157 , 158 , 159 , 500 }}, // Outer Tiles AmpBoard 15
        {{  20 ,  21 ,  22 ,  23 ,  24 ,  25 ,  26 ,  27 , -20 , -21 , -22 , -23 , -24 , -25 , -26 , -27 }}, // B0 Bars 20-27
        {{  28 ,  29 ,  30 ,  31 ,  32 ,   1 ,   2 ,   3 , -28 , -29 , -30 , -31 , -32 ,  -1 ,  -2 ,  -3 }}, // B1 Bars 28-3
        {{ -77 , -76 , -75 , -74 , -73 , -72 , -71 , -70 ,  77 ,  76 ,  75 ,  74 ,  73 ,  72 ,  71 ,  70 }}, // E0 Bars 20-27
        {{ -53 , -52 , -51 , -82 , -81 , -80 , -79 , -78 ,  53 ,  52 ,  51 ,  82 ,  81 ,  80 ,  79 ,  78 }}, // E1 Bars 28-3
        {{ 160 , 161 , 162 , 163 , 164 , 165 , 166 ,-999 , 175 , 176 , 177 , 178 , 179 , 180 , 181 ,-999 }}, // Outer Tiles AmpBoard 12
        {{ 190 , 191 , 192 , 193 , 194 , 195 , 196 , 197 , 205 , 206 , 207 , 208 , 209 , 210 , 211 , 212 }}, // Outer Tiles AmpBoard 13
        {{ 167 , 168 , 169 , 170 , 171 , 172 , 173 , 174 , 182 , 183 , 184 , 185 , 186 , 187 , 188 , 189 }}, // Outer Tiles AmpBoard 16
        {{ 198 , 199 , 200 , 201 , 202 , 203 , 204 ,-999 , 213 , 214 , 215 , 216 , 217 , 218 , 219 , 501 }}, // Outer Tiles AmpBoard 17
        {{-100 ,-101 ,-102 ,-103 ,-104 ,-105 ,-106 ,-107 ,-115 ,-116 ,-117 ,-118 ,-119 ,-120 ,-121 ,-122 }}, // Inner Tiles AmpBoard 18
        {{-130 ,-131 ,-132 ,-133 ,-134 ,-135 ,-136 ,-137 ,-145 ,-146 ,-147 ,-148 ,-149 ,-150 ,-151 ,-152 }}, // Inner Tiles AmpBoard 19
        {{-160 ,-161 ,-162 ,-163 ,-164 ,-165 ,-166 ,-167 ,-175 ,-176 ,-177 ,-178 ,-179 ,-180 ,-181 ,-182 }}, // Inner Tiles AmpBoard 20
        {{-190 ,-191 ,-192 ,-193 ,-194 ,-195 ,-196 ,-197 ,-205 ,-206 ,-207 ,-208 ,-209 ,-210 ,-211 ,-212 }}, // Inner Tiles AmpBoard 21
        {{-108 ,-109 ,-110 ,-111 ,-112 ,-113 ,-114 ,-999 ,-123 ,-124 ,-125 ,-126 ,-127 ,-128 ,-129 ,-999 }}, // Inner Tiles AmpBoard 22
        {{-138 ,-139 ,-140 ,-141 ,-142 ,-143 ,-144 ,-999 ,-153 ,-154 ,-155 ,-156 ,-157 ,-158 ,-159 ,-999 }}, // Inner Tiles AmpBoard 23
        {{-168 ,-169 ,-170 ,-171 ,-172 ,-173 ,-174 ,-999 ,-183 ,-184 ,-185 ,-186 ,-187 ,-188 ,-189 ,-999 }}, // Inner Tiles AmpBoard 24
        {{-198 ,-199 ,-200 ,-201 ,-202 ,-203 ,-204 ,-999 ,-213 ,-214 ,-215 ,-216 ,-217 ,-218 ,-219 , 502 }}, // Inner Tiles AmpBoard 25
        {{ 300 , 301 , 302 , 303 , 304 , 305 , 306 , 307 , 308 , 309 , 310 , 311 , 312 , 313 , 314 , 503 }}, // BGO 1
        {{ 316 , 317 , 318 , 319 , 320 , 321 , 322 , 323 , 324 , 325 , 326 , 327 , 328 , 329 , 330 , 331 }}, // BGO 2
        {{ 332 , 333 , 334 , 335 , 336 , 337 , 338 , 339 , 340 , 341 , 342 , 343 , 344 , 345 , 346 , 600 }}, // BGO 3
        {{ 348 , 349 , 350 , 351 , 352 , 353 , 354 , 355 , 356 , 357 , 358 , 359 , 360 , 361 , 362 , 363 }}  // BGO 4
    }};

    int port = tdcch / 16;
    int index = tdcch % 16;

    return board[port][index];

}

void DataDecoder::flush() {
    if (tree && rootFile) {
        tree->Write("", TObject::kOverwrite);        // ensures tree structure is written
        rootFile->Flush();                           // ensures buffers are flushed to disk
    }
}





// bool DataDecoder::fillData(int channel, int edge, int32_t time, TDCEvent event) {

//     if (edge == 0) {            // TE for signals from FPGA, LE for direct signals

//         if (channel < 0 && channel > -50 ) {    // Inner Downstream Bars

//             channel = channel * -1;
//             if (channel == 32) { channel = 0; }

//             if (event.hodoIDsTE[channel] < 0 ) { event.hodoIDsTE[channel] = 0; }
//             if (event.hodoIDsTE[channel] != 0 && time > event.hodoIDsTE[channel] ) continue;
            
//             event.hodoIDsTE[channel] = time;

//         } else if (channel <= -50 && channel > -90) {   // Inner Upstream Bars

//             channel = (channel + 50) * -1;
//             if (channel == 32) { channel = 0; }

//             if (event.hodoIUsTE[channel] < 0 ) { event.hodoIUsTE[channel] = 0; }
//             if (event.hodoIUsTE[channel] != 0 && time > event.hodoIUsTE[channel] ) continue;
            
//             event.hodoIUsTE[channel] = time;

//         } else if (channel < -90) {     // Inner Tiles
//             channel = (channel + 100) * -1;

//             if (event.tileITE[channel] < 0 ) { event.tileITE[channel] = 0; }
//             if (event.tileITE[channel] != 0 && time > event.tileITE[channel] ) continue;
            
//             event.tileITE[channel] = time;

//         } else if (channel >= 1 && channel < 50){   // Outer Downstream Bars

//             if (channel == 32) { channel = 0; }

//             if (event.hodoODsTE[channel] < 0 ) { event.hodoODsTE[channel] = 0; }
//             if (event.hodoODsTE[channel] != 0 && time > event.hodoODsTE[channel] ) continue;
            
//             event.hodoODsTE[channel] = time;

//         } else if (channel >= 50 && channel < 90) {     // Outer Upstream Bars

//             channel = channel - 50;
//             if (channel == 32) { channel = 0; }

//             if (event.hodoOUsTE[channel] < 0 ) { event.hodoOUsTE[channel] = 0; }
//             if (event.hodoOUsTE[channel] != 0 && time > event.hodoOUsTE[channel] ) continue;
            
//             event.hodoOUsTE[channel] = time;

//         } else if (channel >= 100 && channel < 250) {   // Outer Tiles
            
//             channel = channel - 100; 

//             if (event.tileOTE[channel] < 0 ) { event.tileOTE[channel] = 0; }
//             if (event.tileOTE[channel] != 0 && time > event.tileOTE[channel] ) continue;
            
//             event.tileOTE[channel] = time;

//         } else if (channel >= 300 && channel < 400) {       // BGO

//             channel = channel - 300;

//             if (event.bgoTE[channel] < 0 ) { event.bgoTE[channel] = 0; }
//             if (event.bgoTE[channel] != 0 && time > event.bgoTE[channel] ) continue;
            
//             event.bgoTE[channel] = time;

//         } else if (channel >= 500 && channel < 600) {       // Trigger
//             channel = channel - 500;

//             if (event.trgTE[channel] < 0 ) { event.trgTE[channel] = 0; }
//             if (event.trgTE[channel] != 0 && time > event.trgTE[channel] ) continue;
            
//             event.trgTE[channel] = time;

//         } else if (channel == 600) {        // Gate -- to be removed
//             event.gate = true;
//         }


//     } else if edge == 1 {


//         if (channel < 0 && channel > -50 ) {    // Inner Downstream Bars

//             channel = channel * -1;
//             if (channel == 32) { channel = 0; }

//             if (event.hodoIDsLE[channel] < 0 ) { event.hodoIDsLE[channel] = 0; }
//             if (event.hodoIDsLE[channel] != 0 && time > event.hodoIDsLE[channel] ) continue;
            
//             event.hodoIDsLE[channel] = time;

//         } else if (channel <= -50 && channel > -90) {   // Inner Upstream Bars

//             channel = (channel + 50) * -1;
//             if (channel == 32) { channel = 0; }

//             if (event.hodoIUsLE[channel] < 0 ) { event.hodoIUsLE[channel] = 0; }
//             if (event.hodoIUsLE[channel] != 0 && time > event.hodoIUsLE[channel] ) continue;
            
//             event.hodoIUsLE[channel] = time;

//         } else if (channel < -90) {     // Inner Tiles
//             channel = (channel + 100) * -1;

//             if (event.tileILE[channel] < 0 ) { event.tileILE[channel] = 0; }
//             if (event.tileILE[channel] != 0 && time > event.tileILE[channel] ) continue;
            
//             event.tileILE[channel] = time;

//         } else if (channel >= 1 && channel < 50){   // Outer Downstream Bars

//             if (channel == 32) { channel = 0; }

//             if (event.hodoODsLE[channel] < 0 ) { event.hodoODsLE[channel] = 0; }
//             if (event.hodoODsLE[channel] != 0 && time > event.hodoODsLE[channel] ) continue;
            
//             event.hodoODsLE[channel] = time;

//         } else if (channel >= 50 && channel < 90) {     // Outer Upstream Bars

//             channel = channel - 50;
//             if (channel == 32) { channel = 0; }

//             if (event.hodoOUsLE[channel] < 0 ) { event.hodoOUsLE[channel] = 0; }
//             if (event.hodoOUsLE[channel] != 0 && time > event.hodoOUsLE[channel] ) continue;
            
//             event.hodoOUsLE[channel] = time;

//         } else if (channel >= 100 && channel < 250) {   // Outer Tiles
            
//             channel = channel - 100; 

//             if (event.tileOLE[channel] < 0 ) { event.tileOLE[channel] = 0; }
//             if (event.tileOLE[channel] != 0 && time > event.tileOLE[channel] ) continue;
            
//             event.tileOLE[channel] = time;

//         } else if (channel >= 300 && channel < 400) {   // BGO

//             channel = channel - 300;

//             if (event.bgoLE[channel] < 0 ) { event.bgoLE[channel] = 0; }
//             if (event.bgoLE[channel] != 0 && time > event.bgoLE[channel] ) continue;
            
//             event.bgoLE[channel] = time;

//         } else if (channel >= 500 && channel < 600) {   // Trigger
//             channel = channel - 500;

//             if (event.trgLE[channel] < 0 ) { event.trgLE[channel] = 0; }
//             if (event.trgTE[channel] != 0 && time > event.trgLE[channel] ) continue;
            
//             event.trgLE[channel] = time;

//         } else if (channel == 600) {        // Gate -- to be removed
//             event.gate = true;
//         }


//     }

//     return true;
    
// }