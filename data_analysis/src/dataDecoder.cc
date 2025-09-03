#include <algorithm> // For std::fill_n
#include <cmath>     // For std::nan

#include "dataDecoder.hh"
#include "fileReader.hh"
#include "logger.hh"



// Constructor: Initializes ROOT File & TTree
DataDecoder::DataDecoder(const std::string& outputFile) {
    fileName = outputFile;
    rootFile = new TFile(outputFile.c_str(), "RECREATE");
    tree = new TTree("RawEventTree", "TTree holding the raw Hodoscope Data");

    // Define Tree Branches
    tree->Branch("eventID",         &event.eventID);
    tree->Branch("timestamp",       &event.timestamp);
    tree->Branch("cuspRunNumber",   &event.cuspRunNumber);
    tree->Branch("mixGate",         &event.mixGate);
    tree->Branch("dumpGate",        &event.dumpGate);
    tree->Branch("tdcTimeTag",      &event.tdcTimeTag);
    tree->Branch("fpgaTimeTag",     &event.fpgaTimeTag);
    tree->Branch("trgLE",           &event.trgLE);
    tree->Branch("trgTE",           &event.trgTE);

    tree->Branch("hodoIDsLE",   &event.hodoIDsLE);     // Inner Downstream Leading Edges
    tree->Branch("hodoIUsLE",   &event.hodoIUsLE);     // Inner Upstream Leading Edges
    tree->Branch("hodoODsLE",   &event.hodoODsLE);     // Outer Downstream Leading Edges
    tree->Branch("hodoOUsLE",   &event.hodoOUsLE);     // Outer Upstream Leading Edges
    tree->Branch("hodoIDsTE",   &event.hodoIDsTE);     // Inner Downstream Trailing Edges
    tree->Branch("hodoIUsTE",   &event.hodoIUsTE);     // Inner Upstream Trailing Edges
    tree->Branch("hodoODsTE",   &event.hodoODsTE);     // Outer Downstream Trailing Edges
    tree->Branch("hodoOUsTE",   &event.hodoOUsTE);     // Outer Upstream Trailing Edges
    tree->Branch("bgoLE",       &event.bgoLE);     // BGO Leading Edges
    tree->Branch("bgoTE",       &event.bgoTE);     // BGO Trailing Edges

    tree->Branch("tileILE",     &event.tileILE);     // Tile Inner Leading Edges
    tree->Branch("tileITE",     &event.tileITE);     // Tile Inner Trailing Edges
    tree->Branch("tileOLE",     &event.tileOLE);     // Tile Outer Leading Edges
    tree->Branch("tileOTE",     &event.tileOTE);     // Tile Outer Trailing Edges

    tree->Branch("tdcID",       &event.tdcID);

    tree->SetAutoFlush(10);
    // tree->SetBasketSize("*", 1024);  // reduce basket size
    tree->SetAutoSave(0);

}

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
        // else if (channel == 600) return GATE;                       // Gate
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
            // if (channel == 600) { event.mixGate = true; }
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


    log->trace("Channel: {0:d} | Edge: {1:d} | Time: {2:d} | Type: {3:d}", channel, edge, time, (int)type);
    assignTime(event, channel, edge, time, type);
    return true;
}


uint32_t DataDecoder::processEvent(const char bankName[4], Event& dataevent) {

    auto log = Logger::getLogger();
    uint32_t lastEventID;

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
                event.cuspRunNumber = cuspValue;
                log->trace("[TDC Hit] Bank: {0} | Raw Ch: {1:d} | TDC ID: {2} | Ch: {3:d} | Time: {4:d}", bankN, rawch, tdcID, ch, data_time);
                
            } else if (IS_TDC_HEADER(word)) {

                log->trace("[TDC Header] Bank: {}", bankN);

            } else if (IS_TRIGGER_TIME_TAG(word)) {

                timetag = DATA_MEAS(word);
                log->trace("[TDC ETTT] Bank: {}", bankN);

            } else if (IS_TDC_TRAILER(word)) {

                this_evt[tdcID] = DATA_EVENT_ID(word) + reset_ctr[tdcID] * 0x1000 ;
                if (this_evt[tdcID] < last_evt[tdcID]) {
                    reset_ctr[tdcID]++;
                }
                event.eventID = DATA_EVENT_ID(word) + reset_ctr[tdcID] * 0x1000 ;
                last_evt[tdcID] = event.eventID;

                log->trace("[TDC Trailer] Bank: {} | Event ID: {}", bankN, event.eventID);

            } else if (IS_GLOBAL_HEADER(word)) {

                log->trace("[Global Header]");

            } else if (IS_GLOBAL_TRAILER(word)) {

                geo = ETTT_GEO(word);
                log->trace("[Global Trailer] | GEO: {} | TimeTag: {:d}", geo, timetag*32+geo);
                
                this_timetag[tdcID] = timetag ; // (timetag*32+geo); // + reset_ctr_time[tdcID] * static_cast<Double_t>(0x100000000) ;
                if (this_timetag[tdcID] < last_timetag[tdcID]) {
                    // log->debug("this_timetag: {:d}, last_timetag: {:d}", this_timetag[tdcID], last_timetag[tdcID]);
                    
                    // n_diffs = static_cast<int64_t>(diffSecTime) / static_cast<int64_t>(0x100000000)*25e-9;
                    // log->debug(n_diffs);
                    reset_ctr_time[tdcID]++;
                    // log->debug("reset_ctr_time: {:d}", reset_ctr_time[tdcID]);
                }

                event.tdcTimeTag = static_cast<Double_t>(timetag*32+geo) + static_cast<Double_t>(reset_ctr_time[tdcID] * 4294967296.0);
                log->trace("[Global Trailer] | GEO: {} | TimeTag: {:f}", geo, event.tdcTimeTag);
                last_timetag[tdcID] = timetag; //(timetag*32+geo);  //event.tdcTimeTag;
                event.cuspRunNumber = cuspValue;
                if (dataevent.timestamp64 > 0) {
                    event.timestamp = nsecTime;
                } else {
                    event.timestamp = static_cast<Double_t>(secTime);
                }

                // log->debug("timestamp in TDC: {} = {}", event.timestamp, nsecTime);
                
                event.mixGate = gateValue; 
                tree->Fill();
                lastEventID = event.eventID;
                event.reset();

            } else {
                log->warn("[Unknown Data] Word: {0:b}", word);
            }
        }
    } else if (bankN == "GATE") {
        for (size_t i = 0; i < data.size(); i++) {
            log->trace("[GATE Event] data: {0:x}", data[i]);
            event.eventID = GATE_EVENT(data[i]);
            event.mixGate = (Bool_t)GATE_BOOL(data[i]);
            event.dumpGate = (Bool_t)DUMP_BOOL(data[i]);
            if (dataevent.timestamp64 != 0){
                event.fpgaTimeTag = static_cast<Double_t>(dataevent.timestamp64);
                event.timestamp = nsecTime;
            } else {
                event.timestamp = secTime;
            }
            event.cuspRunNumber = cuspValue;
            log->trace("[GATE Decode] eventID: 0x{0:x}, mixGate: 0x{1:x}, dumpGate: 0x{2:x}, fpgaTimeTag: {3}",
                event.eventID, event.mixGate, event.dumpGate, event.fpgaTimeTag);
            event.tdcID = 4;
            tree->Fill();
            tree->FlushBaskets();
            event.reset();
        }
    } else if (bankN == "CUSP") {
        if (!data.empty()) {
            for (size_t i = 0; i < data.size(); i++) {
            }
            cuspValue = data[0];
            event.cuspRunNumber = cuspValue;
            
            if (dataevent.timestamp64 > 0) {
                nsecTime = static_cast<Double_t>(static_cast<ULong64_t>(dataevent.timestamp64)*1e-9);
                // log->debug("nsecTime {}", nsecTime);
                event.timestamp = nsecTime;
                // log->debug("CUSP event.timestamp {}", event.timestamp);
            } else {
                secTime = static_cast<Double_t>(dataevent.timestamp);
                log->debug("secTime {}", secTime);
                event.timestamp = secTime;
            }

            log->trace("[CUSP Decode] cuspRunNumber: {0}, timeTag: {1}",
                event.cuspRunNumber, event.timestamp);

            
            // diffSecTime = lastSecTime - secTime;
            // event.timestamp = secTime;

            // lastSecTime = secTime;
            // log->debug("[CUSP Event] Value: {0:d} | Time: {1:d}", cuspValue, secTime);
            // tree->Fill();
            // event.reset();
        }
    } else {
        log->warn("Unknown Bank: {0}", bankN);
    }
    
    return lastEventID;

}

// Write TTree to ROOT File
void DataDecoder::writeTree() {
    if(rootFile  && rootFile->IsOpen()) {
        rootFile->cd();
        rootFile->Write();
    } else {
        rootFile = new TFile(getFileName(), "UPDATE");
        rootFile->cd();
        rootFile->Write();
    }
    
    //std::cout << "ROOT file saved.\n";
}

void DataDecoder::closeFile() {
    if(rootFile && rootFile->IsOpen()){
        rootFile->Close();
    }
}

void DataDecoder::openFile() {
    if (!rootFile) {
        rootFile = new TFile(getFileName(), "UPDATE");
    }
}

bool DataDecoder::fsyncFile(const std::string& filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        perror(("open: " + filename).c_str());
        return false;
    }

    if (fsync(fd) == -1) {
        perror(("fsync: " + filename).c_str());
        close(fd);
        return false;
    }

    close(fd);
    return true;
}

Long64_t DataDecoder::checkFileSize(const std::string& filename) {
    TFile file(filename.c_str(), "READ");
    Long64_t expectedSize = file.GetEND();
    file.Close();
    return expectedSize;
}

bool DataDecoder::isFullyWritten(const std::string& filename) {
    struct stat st;
    if (stat(filename.c_str(), &st) != 0) return false;

    Long64_t expectedSize = checkFileSize(filename);

    return st.st_size == expectedSize;
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
        // {{ 300 , 301 , 302 , 303 , 304 , 305 , 306 , 307 , 308 , 309 , 310 , 311 , 312 , 313 , 314 , 315 }}, // BGO 1
        {{ 316 , 317 , 318 , 319 , 320 , 321 , 322 , 323 , 324 , 325 , 326 , 327 , 328 , 329 , 330 , 331 }}, // BGO 2
        // {{ 332 , 333 , 334 , 335 , 336 , 337 , 338 , 339 , 340 , 341 , 342 , 343 , 344 , 345 , 346 , 600 }}, // BGO 3
        {{ 332 , 333 , 334 , 335 , 336 , 337 , 338 , 339 , 340 , 341 , 342 , 343 , 344 , 345 , 346 , 347 }}, // BGO 3
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

void DataDecoder::autoSave() {
    if (tree && rootFile) {
        tree->AutoSave("SaveSelf"); // flush baskets to disk
    }
}

