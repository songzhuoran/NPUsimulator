#pragma once

#include <string>
#include <algorithm>
#include "mv_fifo.h"
using namespace std;

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
        Request(Mv_Fifo_Item mv_fifo_item, Dram_Info dram_info);
        int operator==(Mv_Fifo_Item temp_mv_fifo_item);
        Mv_Fifo_Item _mv_fifo_item;
        string _Dram_addr;
};

class Dram_Request{
    public:
        Dram_Request();
        void init_DRAM_Request(string Dram_addr);
        string _Dram_addr;
};

class L2_Request{
    public:
        L2_Request(Mv_Fifo_Item mv_fifo_item, int start_cycle);
        Mv_Fifo_Item _mv_fifo_item;
        int _return_cycle;
};






