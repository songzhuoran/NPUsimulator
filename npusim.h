#pragma once

#include <string>
#include <algorithm>
#include <math.h>
#include <sstream>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <queue>
#include <map>
#include <list>
#

#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "MultiChannelMemorySystem.h"
#include "Transaction.h"
#include "IniReader.h"

#include "cache.h"
#include "neuralNetworkSim.h"
#define MAX_SIZE_OF_FTMTAB 500
#define SHIFT_CYCLE 1
#define POST_MPBCALC_CYCLE 1  // post mapped buffer calc cycle

using namespace std;
using namespace DRAMSim;

enum FRAME_T {
    I_FRAME = 0, P_FRAME, B_FRAME
};

class Graph {
    public:
        Graph(){}
        int visited[MAX_SIZE_OF_FTMTAB];
        int arc[MAX_SIZE_OF_FTMTAB][MAX_SIZE_OF_FTMTAB];
};

class Npusim {
    private:
        uint64_t systimer;  // system timer, original name: total_cycle
        int numFrames; // num of frames
        bool mvFifoTraversalLabel;
        FRAME_T frame_type_maptab[MAX_SIZE_OF_FTMTAB]; // frame type mapping table
        map<int, int> blkInBframe;       // frame_B_idx, remain_nums
        vector<int> frameValidBuffer;    // neural network process is completed
        vector<int> frameMcValid;        // motion compensation process is completed
        queue<int> Decode_IP_frame_idx;  // IP order
        vector<int> Decode_B_frame_idx;  // B order
        queue<int> ipNnList;             // IP large NN list
        vector<Request> Pending_request_list; // pending request of DRAM
        int maxid_dramReqList;
        vector<pair<string, int> > Dram_request_list;   // hex string
        vector<Mv_Fifo_Item> Mv_Fifo;    // need to init!!(including mv_fifo_item)
        vector<L2_Request> Pending_L2_request_list; // pending request of L2 cache
        MC_Cache MC_L1_Cache, MC_L2_Cache; // L1 cache and L2 cache
        // dram sim
        string systemIniFilename;
        string deviceIniFilename;
        string cacheIniFilename;
        MultiChannelMemorySystem *memorySystem;
        Callback_t *read_cb, *write_cb;
        Transaction *trans;
        map<uint64_t, list<uint64_t> > pendingReadRequests; 
		map<uint64_t, list<uint64_t> > pendingWriteRequests; 
        uint64_t latency;
        
        // Shifter
        queue<pair<Mv_Fifo_Item, uint64_t> > q_shifter;  // pair<Mv_Fifo_Item, finish_time>
        vector<Mv_Fifo_Item> mappedBuffer;
        queue<pair<Mv_Fifo_Item, uint64_t> > q_postMappedBuffer;
        map<int, list<uint64_t> > pendingWriteReqList;
        
        NeuralNetwork largeNet, smallNet;
        list<uint64_t> pendingLnnReadPerLayer, pendingSnnReadPerLayer;
        list<uint64_t> pendingLnnWritePerFrame, pendingSnnWritePerFrame;
        uint64_t lnnWeightStartAddr, snnWeightStartAddr;
        uint64_t lnnRWStartAddr, snnRWStartAddr;
        uint64_t ipDecodedFramesAddr, bMcFramesAddr;
        bool isLargeNetStart, isSmallNetStart;
        bool isLargeNetOk, isSmallNetOk;
        bool isLnnLayerOk, isSnnLayerOk;

        // Data loading and preprocessing
        void load_ibp_label(string path, string b_fname, string p_fname, string i_fname);
        void load_mvs(string filename);
        void decode_order();    // generate decode order
        void sort_mvs();        // sort b-mvs in mvFifo
        void judge_bid_pred();  // judge if a frame is bidirectional prediction frame
        void getNpuBaseAddr();

        // Npu simulator per layer
        int neuralnet_read_sim(NeuralNetwork net, int fidx, int curlayer);
        int neuralnet_write_sim(NeuralNetwork net, int fidx, int curlayer);

        // HEVC decoder simulator
        int decoder_sim(); // simulate hevc decoder

        // Storage system (L1 & L2 cache, DRAM)
        void l1_cache_sim();  // simulate L1 cache
        void l2_cache_sim();  // simulate L2 cache
        void mapping();
        void lnn_execute();
        void snn_execute();
        void decode_execute();
		void add_pending(Transaction *t, uint64_t cycle);
		void read_complete(unsigned id, uint64_t address, uint64_t done_cycle);
		void write_complete(unsigned id, uint64_t address, uint64_t done_cycle);

        uint64_t smallNNTimer;
        int blayer;
        uint64_t largeNNTimer;
        int iplayer;

    public:
        Npusim();   
        void test();   
        void test_v2();   
        void init(string ibpPath, string bFname, string pFname, string mvsFname, string iFname);
        void controller();    // Simulator controller
};