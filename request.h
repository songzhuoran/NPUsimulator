#pragma once

#include <string>
#include <algorithm>
#include "mv_fifo.h"
using namespace std;

class Dram_Info{
    public:
        Dram_Info(int idx, int x, int y){
            _idx = idx;
            _x = x;
            _y = y;
        }
        int _idx;
        int _x;
        int _y;
};

string dec2bin(int dec, int len);
string generate_dram_addr(Dram_Info dram_info);
string bin2hex(const string& bin);

class Request{
    public:
        Request(Mv_Fifo_Item mv_fifo_item, Dram_Info dram_info){
            _mv_fifo_item._b_idx = mv_fifo_item._b_idx;
            _mv_fifo_item._width = mv_fifo_item._width;
            _mv_fifo_item._height = mv_fifo_item._height;
            _mv_fifo_item._src_x = mv_fifo_item._src_x;
            _mv_fifo_item._src_y = mv_fifo_item._src_y;
            _mv_fifo_item._ref_idx = mv_fifo_item._ref_idx;
            _mv_fifo_item._dst_x = mv_fifo_item._dst_x;
            _mv_fifo_item._dst_y = mv_fifo_item._dst_y;
            _Dram_addr = generate_dram_addr(dram_info);
        }
        int operator==(Mv_Fifo_Item temp_mv_fifo_item){
            int flag = 0;
            if(_mv_fifo_item==temp_mv_fifo_item){
                flag = 1;
            }
            return flag;
        }
        Mv_Fifo_Item _mv_fifo_item;
        string _Dram_addr;
};

class Dram_Request{
    public:
        Dram_Request(){}
        void init_DRAM_Request(string Dram_addr){
            _Dram_addr = Dram_addr;
            // _return_cycle = return_cycle;
        }
        // int _return_cycle;
        string _Dram_addr;
};

class L2_Request{
    public:
        L2_Request(Mv_Fifo_Item mv_fifo_item, int start_cycle){
            _mv_fifo_item._b_idx = mv_fifo_item._b_idx;
            _mv_fifo_item._width = mv_fifo_item._width;
            _mv_fifo_item._height = mv_fifo_item._height;
            _mv_fifo_item._src_x = mv_fifo_item._src_x;
            _mv_fifo_item._src_y = mv_fifo_item._src_y;
            _mv_fifo_item._ref_idx = mv_fifo_item._ref_idx;
            _mv_fifo_item._dst_x = mv_fifo_item._dst_x;
            _mv_fifo_item._dst_y = mv_fifo_item._dst_y;
            _return_cycle = start_cycle + 1;
        }
        Mv_Fifo_Item _mv_fifo_item;
        int _return_cycle;
};


// convert dec to bin, the length of bin_num
string dec2bin(int dec, int len) {
	string bin = "";
	while (dec) {
		bin += (dec % 2 + '0');
		dec /= 2;
	}
	while (bin.size() < len) {
		bin += '0';
	}
	reverse(bin.begin(), bin.end());
	return bin;
}

// ref_index = 8-bit, ref_x = 12-bit, ref_y = 12-bit
string generate_dram_addr(Dram_Info dram_info) {
	return dec2bin(dram_info._idx, 8) + dec2bin(dram_info._x, 12)
		+ dec2bin(dram_info._y, 12);
}

// convert bin to hex
string bin2hex(const string& bin) {
    string res = "0x";
    for (int i = 0; i < bin.size(); i += 4) {
        string tmp = bin.substr(i, 4);
        if (tmp[0] == '0') {
            if (tmp == "0000")res += '0';
            else if (tmp == "0001") res += '1';
            else if (tmp == "0010") res += '2';
            else if (tmp == "0011") res += '3';
            else if (tmp == "0100") res += '4';
            else if (tmp == "0101") res += '5';
            else if (tmp == "0110") res += '6';
            else res += '7';
        }
        else {
            if (tmp == "1000") res += '8';
            else if (tmp == "1001") res += '9';
            else if (tmp == "1010") res += 'A';
            else if (tmp == "1011") res += 'B';
            else if (tmp == "1100") res += 'C';
            else if (tmp == "1101") res += 'D';
            else if (tmp == "1110") res += 'E';
            else res += 'F';
        }
    }
    return res;
}
