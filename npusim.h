#pragma once

#include <string>
#include <algorithm>
#include <math.h>
#include <sstream>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <map>
#include <list>

#include "SystemConfiguration.h"
#include "MemorySystem.h"
#include "MultiChannelMemorySystem.h"
#include "Transaction.h"
#include "IniReader.h"

#include "request.h"
#include "cache.h"
#define MAX_SIZE_OF_FTMTAB 500

using namespace std;
using namespace DRAMSim;

enum FRAME_T {
    I_FRAME = 0, P_FRAME, B_FRAME
};

class Graph{
    public:
        Graph(){}
        int visited[MAX_SIZE_OF_FTMTAB];
        int arc[MAX_SIZE_OF_FTMTAB][MAX_SIZE_OF_FTMTAB];
};

class Npusim {
    private:
        int systimer;  // system timer, original name: total_cycle
        int num_frame; // num of frames
        FRAME_T frame_type_maptab[MAX_SIZE_OF_FTMTAB]; // frame type mapping table
        vector<int> Valid_NN_Frame; // neural network process is completed
        vector<int> Valid_MC_Frame; // motion compensation process is completed
        vector<int> Valid_Decode_Frame; // decode process is completed
        vector<int> Decode_frame_idx; // the whole decode order
        vector<int> Decode_IP_frame_idx; // IP order
        vector<int> Decode_B_frame_idx; // B order
        vector<Request> Pending_request_list; // pending request of DRAM
        vector<Dram_Request> Dram_Request_list;
        vector<Mv_Fifo_Item> Mv_Fifo; // need to init!!(including mv_fifo_item)
        vector<L2_Request> Pending_L2_request_list; // pending request of L2 cache
        MC_Cache MC_L1_Cache, MC_L2_Cache;  // L1 cache and L2 cache

        // dram sim
        string systemIniFilename;
        string deviceIniFilename;
        MultiChannelMemorySystem *memorySystem;
        // TransactionReceiver transactionReceiver;
        Callback_t *read_cb, *write_cb;
        Transaction *trans;

        map<uint64_t, list<uint64_t> > pendingReadRequests; 
		map<uint64_t, list<uint64_t> > pendingWriteRequests; 
        uint64_t latency;

        //DAG according to the decode order
        Graph graph;
        


    public:
        int order[MAX_SIZE_OF_FTMTAB];
        Npusim();
        Npusim(MC_Cache _l1_cache_config, MC_Cache _l2_cache_config);
        void load_ibp_label(string path, string b_fname, string p_fname, string i_fname);
        void load_mvs(string filename);
        void judge_bid_pred();  // judge if a frame is bidirectional prediction frame
        int neural_network_sim(FRAME_T frame_type, size_t array_size); // simulate the neural network
        int decoder_sim(); // simulate hevc decoder
        Data_Info generate_cache_addr(int ref_idx, int x); // generate cache data, including tag, index and bank id
        void l1_cache_sim();  // simulate L1 cache
        void l2_cache_sim();  // simulate L2 cache
        void controller();    // main controller of npusim

		void add_pending(Transaction *t, uint64_t cycle);
		void read_complete(unsigned id, uint64_t address, uint64_t done_cycle);
		void write_complete(unsigned id, uint64_t address, uint64_t done_cycle);

        void test();

        //generate DAG
        void generate_DAG();
        void decode_order();

        //sort mvs
        void sort_mvs();
};