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
// #include "dramsim.h"

using namespace std;
using namespace DRAMSim;

enum FRAME_T {
    I_FRAME = 0, P_FRAME, B_FRAME
};

class Npusim {
    private:
        int systimer;  // system timer, original name: total_cycle
        int num_frame; // num of frames
        vector<int> Valid_NN_Frame; // neural network process is completed
        vector<int> Valid_MC_Frame; // motion compensation process is completed
        vector<int> Valid_Decode_Frame; // decode process is completed
        vector<int> Decode_frame_idx; // the whole decode order
        vector<int> Decode_IP_frame_idx; // IP order
        vector<int> Decode_B_frame_idx; // B order
        vector<FRAME_T> Frame_type; //define the frame type of each frame: 0: I frame; 1: P frame; 2: B frame
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

    public:
        Npusim(int _num_frame);
        Npusim(int _num_frame, MC_Cache _l1_cache, MC_Cache _l2_cache);
        int neural_network_sim(FRAME_T frame_type, size_t array_size); // simulate the neural network
        int decoder_sim(); // simulate hevc decoder
        Data_Info generate_cache_addr(int ref_idx, int x); //generate cache data, including tag, index and bank id
        void l1_cache_sim();  // simulate L1 cache
        void l2_cache_sim();  // simulate L2 cache
        // void dram_sim();      // simulate dram
        void controller();    // main controller of npusim

		void add_pending(Transaction *t, uint64_t cycle);
		void read_complete(unsigned id, uint64_t address, uint64_t done_cycle);
		void write_complete(unsigned id, uint64_t address, uint64_t done_cycle);

        void sysini();        // for test case
        // void test_dramsim(int curclk, int clockCycle, string addrStr, TransactionType transactionType);  // test dramsim
};