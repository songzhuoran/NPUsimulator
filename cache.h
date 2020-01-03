#pragma once

#include <string>
#include <vector>
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

class MC_Cache{
    public:
        MC_Cache(){}
        MC_Cache(int cache_bank_x, int cache_bank_y, int cache_set_x, int cache_set_y, int cache_road_x, int cache_road_y, int cache_line_x, int cache_line_y);
        void init_mc_cache(int cache_bank_x, int cache_bank_y, int cache_set_x, int cache_set_y, int cache_road_x, int cache_road_y, int cache_line_x, int cache_line_y);
        void replace_data(Data_Info data_info_x, Data_Info data_info_y);
        void update_frequency();
        bool check_cache_hit(Data_Info data_info_x, Data_Info data_info_y);

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
};