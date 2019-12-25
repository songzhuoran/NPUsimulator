#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
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
        MC_Cache(int cache_bank_x, int cache_bank_y, int cache_set_x, int cache_set_y, int cache_road_x, int cache_road_y, int cache_line_x, int cache_line_y){
            _cache_bank_x = cache_bank_x;
            _cache_bank_y = cache_bank_y;
            _cache_set_x = cache_set_x;
            _cache_set_y = cache_set_y;
            _cache_road_x = cache_road_x;
            _cache_road_y = cache_road_y;
            _cache_line_x = cache_line_x;
            _cache_line_y = cache_line_y;
            _tag_x.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y,-1);
            _tag_y.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y,-1);
            _use_frequency.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y,0);
            _flag_hit.assign(_cache_bank_x*_cache_bank_y*_cache_set_x*_cache_set_y*_cache_road_x*_cache_road_y,1);
        }
        void init_flat_hit(){
            for(int i = 0; i < _flag_hit.size(); i++){
                _flag_hit[i] = 1;
            }
        }
        void replace_data(int cache_bank_x_id, int cache_bank_y_id, int cache_set_x_id, int cache_set_y_id, int tag_x, int tag_y){
            int road_x = 0;
            int road_y = 0;
            int temp_frequency = -1;
            int temp_axis = 0;
            for(;road_x<_cache_road_x;road_x++){
                for(;road_y<_cache_road_y;road_y++){
                    int axis = road_y + road_x * _cache_road_y + cache_set_y_id * _cache_road_x * _cache_road_y  + cache_set_x_id * _cache_set_y * _cache_road_x * _cache_road_y + cache_bank_y_id * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y + cache_bank_x_id * _cache_bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y;
                    if(_use_frequency[axis]>=temp_frequency){
                        temp_frequency = _use_frequency[axis];
                        temp_axis = axis;
                    }
                }
            }
            _tag_x[temp_axis] = tag_x;
            _tag_y[temp_axis] = tag_y; //replace data
            _flag_hit[temp_axis] = 0; //use to update frequency
        }
        void update_frequency(){ //use hit flag to update frequency
            for(int i = 0; i < _flag_hit.size(); i++){
                if(_flag_hit[i]==0){
                    _use_frequency[i] = 0;
                }
                else{
                    _use_frequency[i] = _use_frequency[i] + 1;
                }
            }
        }
        int check_cache_hit(Data_Info data_info_x, Data_Info data_info_y){
            int tag_x = data_info_x._i_tag;
            int tag_y = data_info_y._i_tag;
            int index_x = data_info_x._i_index;
            int index_y = data_info_y._i_index;
            int bank_x = data_info_x._i_b;
            int bank_y = data_info_y._i_b;
            int road_x = 0;
            int road_y = 0;
            int flag_hit = 0;
            for(;road_x<_cache_road_x;road_x++){
                for(;road_y<_cache_road_y;road_y++){
                    int axis = road_y + road_x * _cache_road_y + index_y * _cache_road_x * _cache_road_y  + index_x * _cache_set_y * _cache_road_x * _cache_road_y + bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y + bank_x * _cache_bank_y * _cache_set_x * _cache_set_y * _cache_road_x * _cache_road_y;
                    if(_tag_x[axis] == tag_x && _tag_y[axis] == tag_y){
                        _flag_hit[axis] = 0; //use to update frequency
                        flag_hit = 1;
                    }
                }
            }
            if(flag_hit == true)
                cout << "hit" << endl;
            return flag_hit;
        }
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



class Mv_Fifo_Item{
	public:
        Mv_Fifo_Item(){
            _b_idx=0;
            _width = _height = 8;
            _src_x = _src_y = 0;
            _ref_idx = 0;
            _dst_x = _dst_y = 0;
        }

        Mv_Fifo_Item(int bidx, int w, int h, int srcx, int srcy, int refid, int refx, int refy):_b_idx(bidx), _width(w), _height(h), _src_x(srcx), _src_y(srcy), _ref_idx(refid), _dst_x(refx), _dst_y(refy){}

        void init_mv_fifo_item(int b_idx, int width, int height, int src_x, int src_y, int ref_idx, int dst_x, int dst_y){
            _b_idx = b_idx;
            _width = width;
            _height = height;
            _src_x = src_x;
            _src_y = src_y;
            _ref_idx = ref_idx;
            _dst_x = dst_x;
            _dst_y = dst_y;
        }

        bool operator==(Mv_Fifo_Item temp_mv_item){
            bool flag = false;
            if(_b_idx == temp_mv_item._b_idx && _width == temp_mv_item._width && _height == temp_mv_item._height&&_src_x == temp_mv_item._src_x&&_src_y == temp_mv_item._src_y&&_ref_idx == temp_mv_item._ref_idx&&_dst_x == temp_mv_item._dst_x&&_dst_y == temp_mv_item._dst_y){
                flag = true;
            }
            return flag;
        }

        int _b_idx;
	    int _width;
        int _height;
        int _src_x;
        int _src_y;
        int _ref_idx;
        int _dst_x;
        int _dst_y;
};



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

// dec_num --> bin_num, the length of bin_num
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
        void init_DRAM_Request(string Dram_addr, int return_cycle){
            _Dram_addr = Dram_addr;
            _return_cycle = return_cycle;
        }
        int _return_cycle;
        string _Dram_addr;
};


