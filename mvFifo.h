class Mv_Fifo_Item{
	public:
        Mv_Fifo_Item();

        Mv_Fifo_Item(int bidx, int w, int h, int srcx, int srcy, int refid, int refx, int refy);

        Mv_Fifo_Item(Mv_Fifo_Item* _mv_fifo_item);

        void init_mv_fifo_item(int b_idx, int width, int height, int src_x, int src_y, int ref_idx, int dst_x, int dst_y);

        bool operator==(Mv_Fifo_Item temp_mv_item);

        int _b_idx;
        int _ref_idx;
	int _width;
        int _height;
        int _src_x;
        int _src_y;
        int _dst_x;
        int _dst_y;
        bool isBidPredFrame;  // true：双向预测帧；false：单向预测帧
        int ordmap;
};