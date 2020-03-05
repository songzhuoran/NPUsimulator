#pragma once

#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include<stdint.h>
#include "mvFifo.h"
using namespace std;

// typedef long long uint64_t;

class Dram_Info{
    public:
        Dram_Info(int idx, int x, int y);
        int _idx;
        int _x;
        int _y;
};

string dec2bin(int dec, int len);
string generate_dram_addr(Dram_Info dram_info);
string bin2hex(const string& bin);

class Request{
    public:
        Request(Mv_Fifo_Item _orimv){ 
            _remain_num = 0;
            _ori_mv_item = _orimv;
        }
        Mv_Fifo_Item _ori_mv_item;  // original mv_item without aligning
        vector<pair<Mv_Fifo_Item, int> > _mv_fifo_items;
        int _remain_num;
};

class L2_Request{
    public:
        L2_Request(Mv_Fifo_Item mv_fifo_item, uint64_t start_cycle);
        Mv_Fifo_Item _ori_mv_item;
        vector<Mv_Fifo_Item> _mv_fifo_items;
        uint64_t _return_cycle;
};