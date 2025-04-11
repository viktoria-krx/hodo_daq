#include "dataFilter.hh"
#include <map>
#include "TFile.h"
#include "TTree.h"
#include  "TTreeIndex.h"

#include <ROOT/RDataFrame.hxx>
#include <zmq.hpp>


template <size_t N>
std::array<Double_t, N> computeToT(const std::array<Double_t, N>& le, 
                                   const std::array<Double_t, N>& te) {
    std::array<Double_t, N> tot{};
    for (size_t i = 0; i < N; ++i) {
        tot[i] = (le[i] > 0 && te[i] > 0) ? (te[i] - le[i]) : NAN;
    }
    return tot;
}

template <size_t N>
bool barCoincidence(const std::array<Double_t, N>& ds, 
                                       const std::array<Double_t, N>& us) {
    std::array<bool, N> bc{};
    bool bc_sum = false;
    for (size_t i = 0; i < N; ++i) {
        bc[i] = (ds[i] > 0 && us[i] > 0) ? true : false;
        bc_sum = bc_sum || bc[i];
    }
    return bc_sum;
}

template <size_t N>
int countNonZeroToT(const std::array<Double_t, N>& tot) {
    return std::count_if(tot.begin(), tot.end(), [](Double_t x) { return x > 0; });
}

template <size_t N>
void mergeIfUnset(Double_t (&out)[N], const Double_t (&in)[N]) {
    for (size_t i = 0; i < N; ++i) {
        if (std::isnan(out[i]) && !std::isnan(in[i])) {
            out[i] = in[i];
        }
    }
}

template <size_t N>
void mergeIfUnset(uint32_t (&out)[N], const uint32_t (&in)[N]) {
    for (size_t i = 0; i < N; ++i) {
        if (std::isnan(out[i]) && !std::isnan(in[i])) {
            out[i] = in[i];
        }
    }
}

void mergeIfUnset(uint32_t (&out), const uint32_t (&in)) {
    if (std::isnan(out) && !std::isnan(in)) {
        out = in;
    }
}

void mergeIfUnset(Double_t (&out), const Double_t (&in)) {
    if (std::isnan(out) && !std::isnan(in)) {
        out = in;
    }
}

void mergeIfUnset(Bool_t (&out), const Bool_t (&in)) {
    if (std::isnan(out) && !std::isnan(in)) {
        out = in;
    }
}

void DataFilter::filterAndSend(const char* inputFile) {

    // Set up ZeroMQ Publisher
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PUB);
    socket.bind("tcp://*:5557");  // Publisher socket for sending filtered data

    // Load ROOT file
    ROOT::RDataFrame df("tree", inputFile);

    // Example filter: select events where tdcTimeTag > 1000
    auto filtered_df = df.Filter("bgoLE < " + std::to_string(LE_CUT), "Time Cut")
            .Define("hodoODsToT", computeToT<32>, {"hodoODsLE", "hodoODsTE"})
            .Define("hodoOUsToT", computeToT<32>, {"hodoOUsLE", "hodoOUsTE"})
            .Define("hodoIDsToT", computeToT<32>, {"hodoIDsLE", "hodoIDsTE"})
            .Define("hodoIUsToT", computeToT<32>, {"hodoIUsLE", "hodoIUsTE"})
            .Define("bgoToT", computeToT<64>, {"bgoLE, bgoLE"})
            .Define("bgoCts", countNonZeroToT<64>, {"bgoToT"})
            .Filter(barCoincidence<32>, {"hodoODsToT", "hodoOUsToT"})
            .Filter(barCoincidence<32>, {"hodoODsToT", "hodoOUsToT"})
            .Filter("bgoCts > 1", "BGO Cut");

    auto nEntriesAfterCuts = filtered_df.Count();

    // Process filtered results
    filtered_df.Foreach([&socket](uint32_t eventID, uint32_t tdcTimeTag) {
        std::stringstream ss;
        ss << eventID << " " << tdcTimeTag;

        zmq::message_t message(ss.str().c_str(), ss.str().size());
        socket.send(message, zmq::send_flags::none);
        
    }, {"eventID", "tdcTimeTag"});

    std::cout << "Filtered data sent to ZeroMQ." << std::endl;
}


void DataFilter::fileSorter(const char* inputFile, int last_evt) {
    TFile* file = TFile::Open(inputFile);
    TTree* tree = (TTree*)file->Get("yourTreeName");

    // New output file
    TFile* outputFile = new TFile("merged_sorted.root", "RECREATE");
    TTree* newTree = new TTree("tree", "Merged + Filtered");

    // TTree* newTree = tree->CloneTree(0);

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

    newTree->Branch("eventID",        &eventOut.eventID);
    newTree->Branch("timestamp",      &eventOut.timestamp);
    newTree->Branch("cuspRunNumber",  &eventOut.cuspRunNumber);
    newTree->Branch("gate",           &eventOut.gate);
    newTree->Branch("tdcTimeTag",     &eventOut.tdcTimeTag);
    newTree->Branch("trgLE",          &eventOut.trgLE);
    newTree->Branch("trgTE",          &eventOut.trgTE);

    newTree->Branch("hodoIDsLE",      &eventOut.hodoIDsLE);
    newTree->Branch("hodoIUsLE",      &eventOut.hodoIUsLE);
    newTree->Branch("hodoODsLE",      &eventOut.hodoODsLE);
    newTree->Branch("hodoOUsLE",      &eventOut.hodoOUsLE);
    newTree->Branch("hodoIDsTE",      &eventOut.hodoIDsTE);
    newTree->Branch("hodoIUsTE",      &eventOut.hodoIUsTE);
    newTree->Branch("hodoODsTE",      &eventOut.hodoODsTE);
    newTree->Branch("hodoOUsTE",      &eventOut.hodoOUsTE);
    // newTree->Branch("hodoIDsToT",     &eventOut.hodoIDsToT);
    // newTree->Branch("hodoIUsToT",     &eventOut.hodoIUsToT);
    // newTree->Branch("hodoODsToT",     &eventOut.hodoODsToT);
    // newTree->Branch("hodoOUsToT",     &eventOut.hodoOUsToT);

    newTree->Branch("bgoLE",          &eventOut.bgoLE);
    newTree->Branch("bgoTE",          &eventOut.bgoTE);
    // newTree->Branch("bgoToT",         &eventOut.bgoToT);

    newTree->Branch("tileILE",        &eventOut.tileILE);
    newTree->Branch("tileITE",        &eventOut.tileITE);
    // newTree->Branch("tileIToT",       &eventOut.tileIToT);
    newTree->Branch("tileOLE",        &eventOut.tileOLE);
    newTree->Branch("tileOTE",        &eventOut.tileOTE);
    // newTree->Branch("tileOToT",       &eventOut.tileOToT);

    newTree->Branch("tdcID",          &eventOut.tdcID);
    // newTree->Branch("tdcChannel",     &eventOut.tdcChannel);
    // newTree->Branch("tdcTime",        &eventOut.tdcTime);



    // Map to store merged events by eventID
    std::map<uint32_t, TDCEvent> mergedEvents;

    TTreeIndex* new_ind = new TTreeIndex(tree, "eventID", "tdcID");
    tree->SetTreeIndex(new_ind);
    Int_t nr_evts = (Int_t)tree->GetMaximum("eventID");

    for (Int_t evt = last_evt; evt < nr_evts; evt++){
        eventOut.reset();

        bool validTrigger = false;

        for (Int_t tdc = 0; tdc < 4; tdc++){

            Int_t j = (Int_t) tree->GetEntryNumberWithIndex(evt, tdc);
            if (j < 0) continue;

            tree->GetEntry(j);

            mergeIfUnset(eventOut.trgLE, eventIn.trgLE);
            mergeIfUnset(eventOut.trgTE, eventIn.trgTE);

            mergeIfUnset(eventOut.eventID, eventIn.eventID);
            mergeIfUnset(eventOut.timestamp, eventIn.timestamp);
            mergeIfUnset(eventOut.cuspRunNumber, eventIn.cuspRunNumber);
            mergeIfUnset(eventOut.gate, eventIn.gate);
            mergeIfUnset(eventOut.tdcTimeTag, eventIn.tdcTimeTag);

            switch (tdc) {
                case 0:
                    mergeIfUnset(eventOut.hodoODsLE, eventIn.hodoODsLE);
                    mergeIfUnset(eventOut.hodoODsTE, eventIn.hodoODsTE);
                    mergeIfUnset(eventOut.hodoOUsLE, eventIn.hodoOUsLE);
                    mergeIfUnset(eventOut.hodoOUsTE, eventIn.hodoOUsTE);
                    mergeIfUnset(eventOut.tileOLE,   eventIn.tileOLE);
                    mergeIfUnset(eventOut.tileOTE,   eventIn.tileOTE);
                    break;

                case 1:
                    mergeIfUnset(eventOut.hodoODsLE, eventIn.hodoODsLE);
                    mergeIfUnset(eventOut.hodoODsTE, eventIn.hodoODsTE);
                    mergeIfUnset(eventOut.hodoOUsLE, eventIn.hodoOUsLE);
                    mergeIfUnset(eventOut.hodoOUsTE, eventIn.hodoOUsTE);
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

        newTree->Fill();

    }
}