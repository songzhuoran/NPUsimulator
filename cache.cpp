#include <iostream>
#include <set>
#include "cache.h"

string int2str(int n);
int getPower(int n);
unsigned long bin2dec(string bin);

// init cache
void MC_Cache::init(string filename, CACHE_T t_cache){
    _cache_type = t_cache;
    ifstream iniFile;
    iniFile.open(filename.data());
    if(!iniFile.is_open()){
        cout<<"Cant open cache.ini file!"<<endl;
    }
    string line;
    map<string, int> cache_cfg;
    set<string> cache_list;
    map<CACHE_T, string> cache_type_map;
    cache_type_map[_L1_CACHE] = "L1", cache_type_map[_L2_CACHE] = "L2";
    while(getline(iniFile, line)){
        if(line != "\r"){
            int equpos = line.find('=');
            string key = line.substr(0, equpos);
            key.erase(0, key.find_first_not_of(" "));
            key.erase(key.find_last_not_of(" ") + 1);
            if(key[0] != 'L'){
                cout<<"Invalid naming format! Please check the syntax of \"cache.ini\" "<<endl;
                exit(1);
            }
            cache_list.insert(key.substr(0,2));
            string val = line.substr(equpos + 1);
            val.erase(0, val.find_first_not_of(" "));
            val.erase(val.find_last_not_of(" ") + 1);
            cache_cfg.insert(make_pair(key, atoi(val.c_str())));
        }
    }
    int CONST_CAP;
    for(set<string>::iterator it=cache_list.begin();it!=cache_list.end();it++){
        if(cache_type_map[t_cache]==*it){
            string prefix = *it;
            _cache_bank_x = cache_cfg[prefix+"_CACHE_BANK_X"];
            _cache_bank_y = cache_cfg[prefix+"_CACHE_BANK_Y"];
            _cache_set_x  = cache_cfg[prefix+"_CACHE_SET_X"];
            _cache_set_y  = cache_cfg[prefix+"_CACHE_SET_Y"];
            _cache_road_x = cache_cfg[prefix+"_CACHE_ROAD_X"];
            _cache_road_y = cache_cfg[prefix+"_CACHE_ROAD_Y"];
            _cache_line_x = cache_cfg[prefix+"_CACHE_LINE_X"];
            _cache_line_y = cache_cfg[prefix+"_CACHE_LINE_Y"];

            x_idxlen = getPower(_cache_set_x),  y_idxlen = getPower(_cache_set_y);
            x_banklen = getPower(_cache_bank_x), y_banklen = getPower(_cache_bank_y);
            x_taglen = 9 - x_idxlen - x_banklen, y_taglen = 9 - y_idxlen - y_banklen;

            CONST_CAP = _cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y;
            _tag_x.assign(CONST_CAP, -1);            
            _tag_y.assign(CONST_CAP, -1);
            _use_frequency.assign(CONST_CAP, 0);
            _flag_hit.assign(CONST_CAP, 1);
        }
    }
    DEBUG("== Loading cache model file 'ini/cache.ini' ==");
    int capacity = (CONST_CAP*_cache_line_x*_cache_line_y*2)/ (8*1024);
    DEBUG("===== "<<cache_type_map[t_cache]<<" Cache =====\nCapacity: "<<(capacity>=1024?int2str(capacity/1024)+"MB":int2str(capacity)+"KB")<<" | "<<_cache_bank_x*_cache_bank_y<<" Banks | "<<_cache_set_x*_cache_set_y<<" Sets per bank | "<<_cache_road_x*_cache_road_y<<" Roads per set");
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
    for(int road_x = 0; road_x < _cache_road_x; road_x++) {
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
            if(_tag_x[axis] == tag_x && _tag_y[axis] == tag_y) {
                _flag_hit[axis] = 0; //use to update frequency
                flag_hit = true;
            }
        }
    }
    return flag_hit;
}

// generate cache data, including tag, index and bank id (axis type = 0: x, 1: y)
Data_Info MC_Cache::generate_cache_addr(int ref_idx, int x, int axis_type){
    string str_addr = dec2bin(x, 12);
    Data_Info data_info;
    if(axis_type == 0){
        // axis = x
        string x_tag = dec2bin(ref_idx, 8) + str_addr.substr(0, x_taglen);
        string x_index = str_addr.substr(x_taglen, x_idxlen);
        string x_bank = str_addr.substr(x_taglen + x_idxlen, x_banklen);
        int int_x_tag = bin2dec(x_tag);
        int int_x_index = bin2dec(x_index);
        int int_x_bank = bin2dec(x_bank);
        data_info.init_data_info(int_x_tag, int_x_index, int_x_bank);
    }
    else{
        // axis = y
        string y_tag = dec2bin(ref_idx, 8) + str_addr.substr(0, y_taglen);
        string y_index = str_addr.substr(y_taglen, y_idxlen);
        string y_bank  = str_addr.substr(y_taglen + y_idxlen, y_banklen);
        int int_y_tag = bin2dec(y_tag);
        int int_y_index = bin2dec(y_index);
        int int_y_bank = bin2dec(y_bank);
        data_info.init_data_info(int_y_tag, int_y_index, int_y_bank);
    }
    return data_info;
}

// int to string
string int2str(int n){
    stringstream ss;
    string str;
    ss << n;
    ss >> str;
    return str;
}

// e.g. getPower(4) = 2, getPower(64) = 6, getPower(32) = 5
int getPower(int n){
    int times = 0;
    while(n != 1){
        n >>= 1;
        times++;
    }
    return times;
}

unsigned long bin2dec(string bin) {
    bitset<17> bit(bin);
    return bit.to_ulong();
}