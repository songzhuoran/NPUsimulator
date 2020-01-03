#include <iostream>
#include "cache.h"

// init cache
MC_Cache::MC_Cache(int cache_bank_x, int cache_bank_y, int cache_set_x, int cache_set_y, int cache_road_x, int cache_road_y, int cache_line_x, int cache_line_y){
    _cache_bank_x = cache_bank_x;
    _cache_bank_y = cache_bank_y;
    _cache_set_x = cache_set_x;
    _cache_set_y = cache_set_y;
    _cache_road_x = cache_road_x;
    _cache_road_y = cache_road_y;
    _cache_line_x = cache_line_x;
    _cache_line_y = cache_line_y;
    _tag_x.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, -1);
    _tag_y.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, -1);
    _use_frequency.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, 0);
    _flag_hit.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, 1);
}

void MC_Cache::init_mc_cache(int cache_bank_x, int cache_bank_y, int cache_set_x, int cache_set_y, int cache_road_x, int cache_road_y, int cache_line_x, int cache_line_y){
    _cache_bank_x = cache_bank_x;
    _cache_bank_y = cache_bank_y;
    _cache_set_x = cache_set_x;
    _cache_set_y = cache_set_y;
    _cache_road_x = cache_road_x;
    _cache_road_y = cache_road_y;
    _cache_line_x = cache_line_x;
    _cache_line_y = cache_line_y;
    _tag_x.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, -1);
    _tag_y.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, -1);
    _use_frequency.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, 0);
    _flag_hit.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y, 1);
}

// LRU replacement strategy
void MC_Cache::replace_data(Data_Info data_info_x, Data_Info data_info_y){
    int tag_x = data_info_x._i_tag;
    int tag_y = data_info_y._i_tag;
    int index_x = data_info_x._i_index;
    int index_y = data_info_y._i_index;
    int bank_x = data_info_x._i_b;
    int bank_y = data_info_y._i_b;
    int temp_frequency = -1;
    int temp_axis = 0;
    const int CACHE_IDX_CONST = index_y * _cache_road_x * _cache_road_y  + index_x * _cache_set_y * _cache_road_x * _cache_road_y + bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y + bank_x * _cache_bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y;

    for(int road_x = 0;road_x < _cache_road_x; road_x++) {
        for(int road_y = 0; road_y < _cache_road_y; road_y++) {
            int axis = road_y + road_x * _cache_road_y + CACHE_IDX_CONST;
            if(_use_frequency[axis] >= temp_frequency){
                temp_frequency = _use_frequency[axis];
                temp_axis = axis;
            }
        }
    }
    _tag_x[temp_axis] = tag_x;
    _tag_y[temp_axis] = tag_y; //replace data
    _flag_hit[temp_axis] = 0;  //use to update frequency
}

// use hit flag to update frequency
void MC_Cache::update_frequency(){ 
    for(int i = 0; i < _flag_hit.size(); i++) {
        if(_flag_hit[i] == 0) {
            _use_frequency[i] = 0;
        }
        else {
            _use_frequency[i] = _use_frequency[i] + 1;
        }
    }
}

// judge if the cache hit, hit: return true, miss: return false
bool MC_Cache::check_cache_hit(Data_Info data_info_x, Data_Info data_info_y){
    int tag_x = data_info_x._i_tag;
    int tag_y = data_info_y._i_tag;
    int index_x = data_info_x._i_index;
    int index_y = data_info_y._i_index;
    int bank_x = data_info_x._i_b;
    int bank_y = data_info_y._i_b;
    int flag_hit = false;
    const int CACHE_IDX_CONST = index_y * _cache_road_x * _cache_road_y  + index_x * _cache_set_y * _cache_road_x * _cache_road_y + bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y + bank_x * _cache_bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y;

    for(int road_x = 0; road_x < _cache_road_x; road_x++) {
        for(int road_y = 0; road_y < _cache_road_y; road_y++) {
            int axis = road_y + road_x * _cache_road_y + CACHE_IDX_CONST;
            // cout<<"_tag_x[axis] = "<<_tag_x[axis]<<", tag_x = "<<tag_x<<", _tag_y[axis] = "<<_tag_y[axis]<<", tag_y = "<<tag_y<<endl;
            if(_tag_x[axis] == tag_x && _tag_y[axis] == tag_y) {
                _flag_hit[axis] = 0; //use to update frequency
                flag_hit = true;
            }
        }
    }
    // std::cout<<"from checkhit(): "<<(flag_hit == true?"Hit":"Miss")<<endl;
    return flag_hit;
}