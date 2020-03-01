#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <bitset>
#include "PrintMacros.h"

#include "request.h"
using namespace std;

class Data_Info{
    public:
        Data_Info(){}
        void init_data_info(int i_tag, int i_index, int i_b){
            _i_tag = i_tag;
            _i_index = i_index;
            _i_b = i_b;
        }
        int _i_tag;
        int _i_index;
        int _i_b;
};

enum CACHE_T {
    _L1_CACHE=0, _L2_CACHE
};

class MC_Cache{
    private:
        int x_taglen, y_taglen;
        int x_idxlen, y_idxlen;
        int x_banklen, y_banklen;

        int _cache_bank_x;
        int _cache_bank_y;
        int _cache_set_x;
        int _cache_set_y;
        int _cache_road_x;
        int _cache_road_y;
        int _cache_line_x;
        int _cache_line_y;

        vector<int> _tag_x;
        vector<int> _tag_y;
        vector<int> _use_frequency; //need to init to 0; larger means less use frequency
        vector<int> _flag_hit; //record which cache line is not used, use by replace data; 0: use in this cycle, 1: unuse in this cycle; need to init to 1 at first!
        CACHE_T _cache_type;

    public:
        MC_Cache(){}
        void init(string filename, CACHE_T t_cache);
        void replace_data(Data_Info data_info_x, Data_Info data_info_y);
        void update_frequency();
        bool check_cache_hit(Data_Info data_info_x, Data_Info data_info_y);
        Data_Info generate_cache_addr(int ref_idx, int x, int axis_type); // generate cache data, including tag, index and bank id
};