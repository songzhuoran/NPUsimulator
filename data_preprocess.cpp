#include "codegen.h"

// divide 16*16, 16*8, 8*16 to 8*8 block
vector<mv_fifo_item> block_division(vector<mv_fifo_item>& _srcdata) {
	vector<mv_fifo_item> res;
	int i = 0;
	for (vector<mv_fifo_item>::iterator it = _srcdata.begin();
		it != _srcdata.end(); it++) {
		if (it->height == 16 && it->width == 16) {
			mv_fifo_item tmp_item;
			tmp_item.b_index = it->b_index;
			tmp_item.src_pos.x = it->src_pos.x + 8;
			tmp_item.src_pos.y = it->src_pos.y + 8;
			tmp_item.ref_addr.ref_index = it->ref_addr.ref_index;
			tmp_item.ref_addr.ref_loc.x = it->ref_addr.ref_loc.x + 8;
			tmp_item.ref_addr.ref_loc.y = it->ref_addr.ref_loc.y + 8;
			res.push_back(tmp_item);
		}
		if (it->width == 16) {
			mv_fifo_item tmp_item;
			tmp_item.b_index = it->b_index;
			tmp_item.height = it->height = 8;
			tmp_item.src_pos.x = it->src_pos.x + 8;
			tmp_item.src_pos.y = it->src_pos.y;
			tmp_item.ref_addr.ref_index = it->ref_addr.ref_index;
			tmp_item.ref_addr.ref_loc.x = it->ref_addr.ref_loc.x + 8;
			tmp_item.ref_addr.ref_loc.y = it->ref_addr.ref_loc.y;
			res.push_back(tmp_item);
		}
		if (it->height == 16) {
			mv_fifo_item tmp_item;
			tmp_item.b_index = it->b_index;
			tmp_item.width = it->width = 8;
			tmp_item.src_pos.x = it->src_pos.x;
			tmp_item.src_pos.y = it->src_pos.y + 8;
			tmp_item.ref_addr.ref_index = it->ref_addr.ref_index;
			tmp_item.ref_addr.ref_loc.x = it->ref_addr.ref_loc.x;
			tmp_item.ref_addr.ref_loc.y = it->ref_addr.ref_loc.y + 8;
			res.push_back(tmp_item);
		}
		res.push_back(*it);

		reverse(res.begin() + i, res.end());
		i = res.size();
	}
	return res;
}

