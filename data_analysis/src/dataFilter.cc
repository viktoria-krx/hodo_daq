#include "dataFilter.hh"
#include <map>
#include "TFile.h"
#include "TTree.h"
#include  "TTreeIndex.h"

#include <ROOT/RDataFrame.hxx>
#include <zmq.hpp>


template <size_t N>
ROOT::VecOps::RVec<Double_t> computeToT(const ROOT::RVec<Double_t>& le, 
                                        const ROOT::RVec<Double_t>& te) {
    ROOT::VecOps::RVec<Double_t> tot(N);
    for (size_t i = 0; i < N; ++i) {
        tot[i] = (le[i] > 0 && te[i] > 0) ? (te[i] - le[i]) : NAN;
    }
    return tot;
}

template <size_t N>
bool barCoincidence(const ROOT::RVec<Double_t>& ds, 
                    const ROOT::RVec<Double_t>& us) {
    bool coincidence = false;
    for (size_t i = 0; i < N; ++i) {
        if (ds[i] > 0 && us[i] > 0) {
            coincidence = true;
            break;
        }
    }
    return coincidence;
}

template <size_t N>
int countNonZeroToT(const ROOT::RVec<Double_t>& tot) {
    int count = 0;
    for (size_t i = 0; i < N; ++i) {
        if (tot[i] > 0) {
            ++count;
        }
    }
    return count;
}

template <size_t N>
std::vector<int> getActiveIndices(const ROOT::RVec<Double_t>& arr) {
    std::vector<int> indices;
    for (size_t i = 0; i < N; ++i) {
        if (!std::isnan(arr[i]) && arr[i] > 0) {
            indices.push_back(i);
        }
    }
    return indices;
}

template <size_t N>
void mergeIfUnset(std::array<Double_t, N>& out, const std::array<Double_t, N>& in) {
    for (size_t i = 0; i < N; ++i) {
        if (std::isnan(out[i]) && !std::isnan(in[i])) {
            out[i] = in[i];
        }
    }
}

template <size_t N>
void mergeIfUnset(std::array<uint32_t, N>& out, const std::array<uint32_t, N>& in) {
    for (size_t i = 0; i < N; ++i) {
        if (out[i] == UINT32_UNSET && in[i] != UINT32_UNSET) {
            out[i] = in[i];
        }
    }
}

void mergeIfUnset(uint32_t& out, const uint32_t& in) {
    if (out == UINT32_UNSET && in != UINT32_UNSET) {
        out = in;
    }
}

void mergeIfUnset(Double_t& out, const Double_t& in) {
    if (std::isnan(out) && !std::isnan(in)) {
        out = in;
    }
}

void mergeIfUnset(Bool_t& out, const Bool_t& in) {
    if (out == BOOL_UNSET && in != BOOL_UNSET) {
        out = in;
    }
}

void DataFilter::fileSorter(const char* inputFile, int last_evt, const char* outputFile) {
    TFile* file = TFile::Open(inputFile);
    TTree* tree = (TTree*)file->Get("RawEventTree");
    
    // New output file
    TFile* output;
    if (last_evt == 0){
        output = new TFile(outputFile, "RECREATE");
    } else {
        output = new TFile(outputFile, "UPDATE");
    }
    
    TTree* newTree = (TTree*)output->Get("RawEventTree");

    TDCEvent eventIn;

    tree->SetBranchAddress("eventID",        &eventIn.eventID);
    tree->SetBranchAddress("timestamp",      &eventIn.timestamp);
    tree->SetBranchAddress("cuspRunNumber",  &eventIn.cuspRunNumber);
    tree->SetBranchAddress("gate",           &eventIn.gate);
    tree->SetBranchAddress("tdcTimeTag",     &eventIn.tdcTimeTag);
    tree->SetBranchAddress("trgLE",          &eventIn.trgLE);
    tree->SetBranchAddress("trgTE",          &eventIn.trgTE);

    tree->SetBranchAddress("hodoIDsLE",      &eventIn.hodoIDsLE);
    tree->SetBranchAddress("hodoIUsLE",      &eventIn.hodoIUsLE);
    tree->SetBranchAddress("hodoODsLE",      &eventIn.hodoODsLE);
    tree->SetBranchAddress("hodoOUsLE",      &eventIn.hodoOUsLE);
    tree->SetBranchAddress("hodoIDsTE",      &eventIn.hodoIDsTE);
    tree->SetBranchAddress("hodoIUsTE",      &eventIn.hodoIUsTE);
    tree->SetBranchAddress("hodoODsTE",      &eventIn.hodoODsTE);
    tree->SetBranchAddress("hodoOUsTE",      &eventIn.hodoOUsTE);
    // tree->SetBranchAddress("hodoIDsToT",     &eventIn.hodoIDsToT);
    // tree->SetBranchAddress("hodoIUsToT",     &eventIn.hodoIUsToT);
    // tree->SetBranchAddress("hodoODsToT",     &eventIn.hodoODsToT);
    // tree->SetBranchAddress("hodoOUsToT",     &eventIn.hodoOUsToT);

    tree->SetBranchAddress("bgoLE",          &eventIn.bgoLE);
    tree->SetBranchAddress("bgoTE",          &eventIn.bgoTE);
    // tree->SetBranchAddress("bgoToT",         &eventIn.bgoToT);

    tree->SetBranchAddress("tileILE",        &eventIn.tileILE);
    tree->SetBranchAddress("tileITE",        &eventIn.tileITE);
    // tree->SetBranchAddress("tileIToT",       &eventIn.tileIToT);
    tree->SetBranchAddress("tileOLE",        &eventIn.tileOLE);
    tree->SetBranchAddress("tileOTE",        &eventIn.tileOTE);
    // tree->SetBranchAddress("tileOToT",       &eventIn.tileOToT);

    tree->SetBranchAddress("tdcID",          &eventIn.tdcID);
    // tree->SetBranchAddress("tdcChannel",     &eventIn.tdcChannel);
    // tree->SetBranchAddress("tdcTime",        &eventIn.tdcTime);


    TDCEvent eventOut;
    
    if (!newTree) {
        newTree = new TTree("RawEventTree", "TDCs Merged");

        newTree->Branch("eventID",        &eventOut.eventID,            "eventID/i");
        newTree->Branch("timestamp",      &eventOut.timestamp,          "timestamp/D");
        newTree->Branch("cuspRunNumber",  &eventOut.cuspRunNumber,      "cuspRunNumber/i"  );
        newTree->Branch("gate",           &eventOut.gate,               "gate/O");
        newTree->Branch("tdcTimeTag",     &eventOut.tdcTimeTag,         "tdcTimeTag/D");
        newTree->Branch("trgLE",          eventOut.trgLE.data(),        "trgLE[4]/D");
        newTree->Branch("trgTE",          eventOut.trgTE.data(),        "trgTE[4]/D");

        newTree->Branch("hodoIDsLE",      eventOut.hodoIDsLE.data(),    "hodoIDsLE[32]/D");
        newTree->Branch("hodoIUsLE",      eventOut.hodoIUsLE.data(),    "hodoIUsLE[32]/D");
        newTree->Branch("hodoODsLE",      eventOut.hodoODsLE.data(),    "hodoODsLE[32]/D");
        newTree->Branch("hodoOUsLE",      eventOut.hodoOUsLE.data(),    "hodoOUsLE[32]/D");
        newTree->Branch("hodoIDsTE",      eventOut.hodoIDsTE.data(),    "hodoIDsTE[32]/D");
        newTree->Branch("hodoIUsTE",      eventOut.hodoIUsTE.data(),    "hodoIUsTE[32]/D");
        newTree->Branch("hodoODsTE",      eventOut.hodoODsTE.data(),    "hodoODsTE[32]/D");
        newTree->Branch("hodoOUsTE",      eventOut.hodoOUsTE.data(),    "hodoOUsTE[32]/D");
        // newTree->Branch("hodoIDsToT",     &eventOut.hodoIDsToT);
        // newTree->Branch("hodoIUsToT",     &eventOut.hodoIUsToT);
        // newTree->Branch("hodoODsToT",     &eventOut.hodoODsToT);
        // newTree->Branch("hodoOUsToT",     &eventOut.hodoOUsToT);

        newTree->Branch("bgoLE",          eventOut.bgoLE.data(),        "bgoLE[64]/D");
        newTree->Branch("bgoTE",          eventOut.bgoTE.data(),        "bgoTE[64]/D");
        // newTree->Branch("bgoToT",         &eventOut.bgoToT);

        newTree->Branch("tileILE",        eventOut.tileILE.data(),      "tileILE[120]/D");
        newTree->Branch("tileITE",        eventOut.tileITE.data(),      "tileITE[120]/D");
        // newTree->Branch("tileIToT",       &eventOut.tileIToT);
        newTree->Branch("tileOLE",        eventOut.tileOLE.data(),      "tileOLE[120]/D");
        newTree->Branch("tileOTE",        eventOut.tileOTE.data(),      "tileOTE[120]/D");
        // newTree->Branch("tileOToT",       &eventOut.tileOToT);

        newTree->Branch("tdcID",          &eventOut.tdcID,              "tdcID[4]/i");
        // newTree->Branch("tdcChannel",     &eventOut.tdcChannel);
        // newTree->Branch("tdcTime",        &eventOut.tdcTime);

    } else {
        newTree->SetBranchAddress("eventID",        &eventOut.eventID);
        newTree->SetBranchAddress("timestamp",      &eventOut.timestamp);
        newTree->SetBranchAddress("cuspRunNumber",  &eventOut.cuspRunNumber);
        newTree->SetBranchAddress("gate",           &eventOut.gate);
        newTree->SetBranchAddress("tdcTimeTag",     &eventOut.tdcTimeTag);
        newTree->SetBranchAddress("trgLE",          eventOut.trgLE.data());
        newTree->SetBranchAddress("trgTE",          eventOut.trgTE.data());

        newTree->SetBranchAddress("hodoIDsLE",      eventOut.hodoIDsLE.data());
        newTree->SetBranchAddress("hodoIUsLE",      eventOut.hodoIUsLE.data());
        newTree->SetBranchAddress("hodoODsLE",      eventOut.hodoODsLE.data());
        newTree->SetBranchAddress("hodoOUsLE",      eventOut.hodoOUsLE.data());
        newTree->SetBranchAddress("hodoIDsTE",      eventOut.hodoIDsTE.data());
        newTree->SetBranchAddress("hodoIUsTE",      eventOut.hodoIUsTE.data());
        newTree->SetBranchAddress("hodoODsTE",      eventOut.hodoODsTE.data());
        newTree->SetBranchAddress("hodoOUsTE",      eventOut.hodoOUsTE.data());
        // newTree->SetBranchAddress("hodoIDsToT",     &eventOut.hodoIDsToT);
        // newTree->SetBranchAddress("hodoIUsToT",     &eventOut.hodoIUsToT);
        // newTree->SetBranchAddress("hodoODsToT",     &eventOut.hodoODsToT);
        // newTree->SetBranchAddress("hodoOUsToT",     &eventOut.hodoOUsToT);

        newTree->SetBranchAddress("bgoLE",          eventOut.bgoLE.data());
        newTree->SetBranchAddress("bgoTE",          eventOut.bgoTE.data());
        // newTree->SetBranchAddress("bgoToT",         &eventOut.bgoToT);

        newTree->SetBranchAddress("tileILE",        eventOut.tileILE.data());
        newTree->SetBranchAddress("tileITE",        eventOut.tileITE.data());
        // newTree->SetBranchAddress("tileIToT",       &eventOut.tileIToT);
        newTree->SetBranchAddress("tileOLE",        eventOut.tileOLE.data());
        newTree->SetBranchAddress("tileOTE",        eventOut.tileOTE.data());
        // newTree->SetBranchAddress("tileOToT",       &eventOut.tileOToT);

        newTree->SetBranchAddress("tdcID",          &eventOut.tdcID);
        // newTree->SetBranchAddress("tdcChannel",     &eventOut.tdcChannel);
        // newTree->SetBranchAddress("tdcTime",        &eventOut.tdcTime);

    }

    // Map to store merged events by eventID
    std::map<uint32_t, TDCEvent> mergedEvents;

    TTreeIndex* new_ind = new TTreeIndex(tree, "eventID", "tdcID");
    tree->SetTreeIndex(new_ind);
    Int_t nr_evts = (Int_t)tree->GetMaximum("eventID");

    for (Int_t evt = last_evt; evt < nr_evts; evt++){
        eventOut.reset();

        for (Int_t tdc = 0; tdc < 4; tdc++){

            Int_t j = (Int_t) tree->GetEntryNumberWithIndex(evt, tdc);
            if (j < 0) continue;

            tree->GetEntry(j);

            mergeIfUnset(eventOut.trgLE,         eventIn.trgLE);
            mergeIfUnset(eventOut.trgTE,         eventIn.trgTE);

            mergeIfUnset(eventOut.eventID,       eventIn.eventID);
            mergeIfUnset(eventOut.timestamp,     eventIn.timestamp);
            mergeIfUnset(eventOut.cuspRunNumber, eventIn.cuspRunNumber);
            mergeIfUnset(eventOut.gate,          eventIn.gate);
            mergeIfUnset(eventOut.tdcTimeTag,    eventIn.tdcTimeTag);
            mergeIfUnset(eventOut.tdcID,         eventIn.tdcID);

            switch (tdc) {
                case 0:
                    mergeIfUnset(eventOut.hodoODsLE, eventIn.hodoODsLE);
                    mergeIfUnset(eventOut.hodoODsTE, eventIn.hodoODsTE);
                    mergeIfUnset(eventOut.hodoOUsLE, eventIn.hodoOUsLE);
                    mergeIfUnset(eventOut.hodoOUsTE, eventIn.hodoOUsTE);
                    mergeIfUnset(eventOut.hodoIDsLE, eventIn.hodoIDsLE);
                    mergeIfUnset(eventOut.hodoIDsTE, eventIn.hodoIDsTE);
                    mergeIfUnset(eventOut.hodoIUsLE, eventIn.hodoIUsLE);
                    mergeIfUnset(eventOut.hodoIUsTE, eventIn.hodoIUsTE);
                    mergeIfUnset(eventOut.tileOLE,   eventIn.tileOLE);
                    mergeIfUnset(eventOut.tileOTE,   eventIn.tileOTE);
                    break;

                case 1:
                    mergeIfUnset(eventOut.hodoODsLE, eventIn.hodoODsLE);
                    mergeIfUnset(eventOut.hodoODsTE, eventIn.hodoODsTE);
                    mergeIfUnset(eventOut.hodoOUsLE, eventIn.hodoOUsLE);
                    mergeIfUnset(eventOut.hodoOUsTE, eventIn.hodoOUsTE);
                    mergeIfUnset(eventOut.hodoIDsLE, eventIn.hodoIDsLE);
                    mergeIfUnset(eventOut.hodoIDsTE, eventIn.hodoIDsTE);
                    mergeIfUnset(eventOut.hodoIUsLE, eventIn.hodoIUsLE);
                    mergeIfUnset(eventOut.hodoIUsTE, eventIn.hodoIUsTE);
                    mergeIfUnset(eventOut.tileOLE,   eventIn.tileOLE);
                    mergeIfUnset(eventOut.tileOTE,   eventIn.tileOTE);
                    break;

                case 2:
                    mergeIfUnset(eventOut.tileILE,   eventIn.tileILE);
                    mergeIfUnset(eventOut.tileITE,   eventIn.tileITE);
                    break;

                case 3:
                    mergeIfUnset(eventOut.bgoLE,     eventIn.bgoLE);
                    mergeIfUnset(eventOut.bgoTE,     eventIn.bgoTE);
                    break;
            }

        }

        convertTime(eventOut);
        newTree->Fill();

    }

    output->cd();
    newTree->Write("", TObject::kOverwrite);
    output->Close();

}

void DataFilter::convertTime(TDCEvent& event) {
    Double_t ns = 0.1;
    Double_t clock_ns = 25;
    for (int i = 0; i < 32; i++) {
        event.hodoIDsLE[i] *= ns;
        event.hodoIDsTE[i] *= ns;
        event.hodoIUsLE[i] *= ns;
        event.hodoIUsTE[i] *= ns;
        event.hodoODsLE[i] *= ns;
        event.hodoODsTE[i] *= ns;
        event.hodoOUsLE[i] *= ns;
        event.hodoOUsTE[i] *= ns;
    }
    for (int i = 0; i < 120; i++) {
        event.tileILE[i] *= ns;
        event.tileITE[i] *= ns;
        event.tileOLE[i] *= ns;
        event.tileOTE[i] *= ns;
    }
    for (int i = 0; i < 64; i++) {
        event.bgoLE[i] *= ns;
        event.bgoTE[i] *= ns;
    }
    for (int i = 0; i < 4; i++) {
        event.trgLE[i] *= ns;
        event.trgTE[i] *= ns;
    }
    event.tdcTimeTag *= clock_ns;
    
}


void DataFilter::filterAndSave(const char* inputFile, int last_evt) {

    // Load ROOT file
    ROOT::RDataFrame df("RawEventTree", inputFile);

    // Example filter: select events where tdcTimeTag > 1000
    auto filtered_df = df.Filter("eventID > " + std::to_string(last_evt), "New Events")
            //.Filter("bgoLE < " + std::to_string(LE_CUT), "Time Cut")
            .Define("hodoODsToT", computeToT<32>, {"hodoODsLE", "hodoODsTE"})
            .Define("hodoOUsToT", computeToT<32>, {"hodoOUsLE", "hodoOUsTE"})
            .Define("hodoIDsToT", computeToT<32>, {"hodoIDsLE", "hodoIDsTE"})
            .Define("hodoIUsToT", computeToT<32>, {"hodoIUsLE", "hodoIUsTE"})
            .Define("bgoToT", computeToT<64>, {"bgoLE", "bgoTE"})
            .Define("bgoCts", countNonZeroToT<64>, {"bgoToT"})
            .Filter(barCoincidence<32>, {"hodoODsToT", "hodoOUsToT"})
            .Filter(barCoincidence<32>, {"hodoIDsToT", "hodoIUsToT"})
            .Define("hodoODsCts", countNonZeroToT<32>, {"hodoODsToT"})
            .Define("hodoOUsCts", countNonZeroToT<32>, {"hodoOUsToT"})
            .Define("hodoIDsCts", countNonZeroToT<32>, {"hodoIDsToT"})
            .Define("hodoIUsCts", countNonZeroToT<32>, {"hodoIUsToT"})
            //.Filter("bgoCts > 1", "BGO Cut")      // uncomment if BGO is used
            .Define("bgoToT_ActiveIndices", getActiveIndices<64>, {"bgoToT"})
            .Define("hodoODsToT_ActiveIndices", getActiveIndices<32>, {"hodoODsToT"})
            .Define("hodoOUsToT_ActiveIndices", getActiveIndices<32>, {"hodoOUsToT"})
            .Define("hodoIDsToT_ActiveIndices", getActiveIndices<32>, {"hodoIDsToT"})
            .Define("hodoIUsToT_ActiveIndices", getActiveIndices<32>, {"hodoIUsToT"})
            ;

    auto nEntriesAfterCuts = filtered_df.Count();

    ROOT::RDF::RSnapshotOptions opts;
    opts.fMode = "update";
    filtered_df.Snapshot("EventTree", inputFile, "", opts);


}

void DataFilter::filterAndSend(const char* inputFile, int last_evt) {

    // Set up ZeroMQ Publisher
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PUB);
    socket.bind("tcp://*:5557");  // Publisher socket for sending filtered data

    // Load ROOT file
    ROOT::RDataFrame df("tree", inputFile);

    // Example filter: select events where tdcTimeTag > 1000
    auto filtered_df = df.Filter("eventID > " + std::to_string(last_evt), "New Events")
            .Filter("bgoLE < " + std::to_string(LE_CUT), "Time Cut")
            .Define("hodoODsToT", computeToT<32>, {"hodoODsLE", "hodoODsTE"})
            .Define("hodoOUsToT", computeToT<32>, {"hodoOUsLE", "hodoOUsTE"})
            .Define("hodoIDsToT", computeToT<32>, {"hodoIDsLE", "hodoIDsTE"})
            .Define("hodoIUsToT", computeToT<32>, {"hodoIUsLE", "hodoIUsTE"})
            .Define("bgoToT", computeToT<64>, {"bgoLE, bgoLE"})
            .Define("bgoCts", countNonZeroToT<64>, {"bgoToT"})
            .Filter(barCoincidence<32>, {"hodoODsToT", "hodoOUsToT"})
            .Filter(barCoincidence<32>, {"hodoODsToT", "hodoOUsToT"})
            .Filter("bgoCts > 1", "BGO Cut")
            .Define("bgoToT_ActiveIndices", getActiveIndices<64>, {"bgoToT"});

    auto nEntriesAfterCuts = filtered_df.Count();

    // Process filtered results
    filtered_df.Foreach([&socket](uint32_t eventID, uint32_t tdcTimeTag, const std::vector<int>& activeBGO) {
        std::stringstream ss;
        ss << eventID << " " << tdcTimeTag;

        for (int ch : activeBGO) {
            ss << " " << ch;
        }

        zmq::message_t message(ss.str().c_str(), ss.str().size());
        socket.send(message, zmq::send_flags::none);

    }, {"eventID", "tdcTimeTag", "bgoToT_ActiveIndices"});

    std::cout << "Filtered data sent to ZeroMQ." << std::endl;
}