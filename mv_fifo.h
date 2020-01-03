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