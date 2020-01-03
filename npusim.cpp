#include "npusim.h"

// init Npusim
Npusim::Npusim(int _num_frame){
    num_frame = _num_frame;
    systimer = 0;
    Valid_NN_Frame.assign(num_frame, 0);
    Valid_MC_Frame.assign(num_frame, 0);
    Valid_Decode_Frame.assign(num_frame, 0);
    MC_L1_Cache.init_mc_cache(2, 2, 1, 1, 4, 4, 8, 8);
    MC_L2_Cache.init_mc_cache(2, 2, 1, 1, 8, 8, 8, 8);
    systemIniFilename = "system.ini";
    deviceIniFilename = "ini/DDR3_micron_64M_8B_x4_sg15.ini";
    memorySystem = new MultiChannelMemorySystem(deviceIniFilename, systemIniFilename, "", "", 2048, NULL, NULL);
    memorySystem->setCPUClockSpeed(0); 	// set the frequency ratio to 1:1
    read_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::read_complete);
    write_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::write_complete);
	memorySystem->RegisterCallbacks(read_cb, write_cb, NULL);
    trans = NULL;
}

Npusim::Npusim(int _num_frame, MC_Cache _l1_cache, MC_Cache _l2_cache){
    num_frame = _num_frame;
    systimer = 0;
    Valid_NN_Frame.assign(num_frame, 0);
    Valid_MC_Frame.assign(num_frame, 0);
    Valid_Decode_Frame.assign(num_frame, 0);
    MC_L1_Cache = _l1_cache;
    MC_L2_Cache = _l2_cache;
    systemIniFilename = "system.ini";
    deviceIniFilename = "ini/DDR3_micron_64M_8B_x4_sg15.ini";
    memorySystem = new MultiChannelMemorySystem(deviceIniFilename, systemIniFilename, "", "", 2048, NULL, NULL);
    memorySystem->setCPUClockSpeed(0); 	// set the frequency ratio to 1:1
    read_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::read_complete);
    write_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::write_complete);
	memorySystem->RegisterCallbacks(read_cb, write_cb, NULL);
    trans = NULL;
}

// simulate the neural network
int Npusim::neural_network_sim(FRAME_T frame_type, size_t array_size=128*128){ 
    int NN_cycle = 0;
    switch (frame_type)
    {
    case I_FRAME:
        //NN_cycle = 3805503; //‭62349373440/(128*128)
        NN_cycle = 100; //test case
        break;
    case P_FRAME:
        //NN_cycle = 3805503; //need to add
        NN_cycle = 100; //test case
        break;
    case B_FRAME:
        //NN_cycle = 380550; //need to add
        NN_cycle = 10; //test case
        break;
    }
    return NN_cycle;
}

// generate cache data, including tag, index and bank id
Data_Info Npusim::generate_cache_addr(int ref_idx, int x){ 
    string str_x0 = dec2bin(x, 12);
    string tag_x0 = dec2bin(ref_idx, 8) + str_x0.substr(0, 6);
    string index_x0 = str_x0.substr(6,2);
    string bank_x0 = str_x0.substr(8,1);

    int i_tag_x0 = atoi(tag_x0.c_str());
    int i_index_x0 = atoi(index_x0.c_str());
    int i_bank_x0 = atoi(bank_x0.c_str());
    Data_Info data_info;
    data_info.init_data_info(i_tag_x0,i_index_x0,i_bank_x0);
    return data_info;
}

// simulate hevc decoder
int Npusim::decoder_sim(){
    int frequency = 3; //300MHz
    int FPS = 6; //60fps
    int cycle = 3/6 * pow(10,7);
    return cycle;
}

void Npusim::add_pending(Transaction *t, uint64_t cycle)
{
	// C++ lists are ordered, so the list will always push to the back and
	// remove at the front to ensure ordering
	if (t->transactionType == DATA_READ)
	{
		pendingReadRequests[t->address].push_back(cycle); 
	}
	else if (t->transactionType == DATA_WRITE)
	{
		pendingWriteRequests[t->address].push_back(cycle); 
	}
	else
	{
		ERROR("This should never happen"); 
		exit(-1);
	}
}

void Npusim::read_complete(unsigned id, uint64_t address, uint64_t done_cycle)
{
	map<uint64_t, list<uint64_t> >::iterator it;
	it = pendingReadRequests.find(address); 
	if (it == pendingReadRequests.end())
	{
		ERROR("Cant find a pending read for this one"); 
		exit(-1);
	}
	else
	{
		if (it->second.size() == 0)
		{
			ERROR("Nothing here, either"); 
			exit(-1); 
		}
	}

	uint64_t added_cycle = pendingReadRequests[address].front();
	latency = done_cycle - added_cycle;

	pendingReadRequests[address].pop_front();
	cout << "Read Callback:  0x"<< std::hex << std::setw(8) << std::setfill('0') << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
    
    ostringstream b;
    b << std::setw(8) << std::setfill('0') << hex << address;
    string addrStr = "0x" + b.str();
    
    for (int i = 0; i < Dram_Request_list.size();) {
        if (bin2hex(Dram_Request_list[i]._Dram_addr) == addrStr) {
            for (int j = 0; j < Pending_request_list.size();) {
                if (Dram_Request_list[i]._Dram_addr == Pending_request_list[j]._Dram_addr) {
                    int tmp_idx = Pending_request_list[j]._mv_fifo_item._ref_idx;
                    int tmp_x = Pending_request_list[j]._mv_fifo_item._dst_x;
                    int tmp_y = Pending_request_list[j]._mv_fifo_item._dst_y;
                    Data_Info tmp_x0_info;
                    Data_Info tmp_y0_info;
                    tmp_x0_info = generate_cache_addr(tmp_idx, tmp_x);
                    tmp_y0_info = generate_cache_addr(tmp_idx, tmp_y);
                    MC_L1_Cache.replace_data(tmp_x0_info,tmp_y0_info);
                    MC_L2_Cache.replace_data(tmp_x0_info,tmp_y0_info);
                    Pending_request_list.erase(Pending_request_list.begin() + j);
                }
                else{
                    j++;
                }
            }
            Dram_Request_list.erase(Dram_Request_list.begin() + i);
            break;
        }
        else i++;
    }
}

void Npusim::write_complete(unsigned id, uint64_t address, uint64_t done_cycle)
{
	map<uint64_t, list<uint64_t> >::iterator it;
	it = pendingWriteRequests.find(address); 
	if (it == pendingWriteRequests.end())
	{
		ERROR("Cant find a pending read for this one"); 
		exit(-1);
	}
	else
	{
		if (it->second.size() == 0)
		{
			ERROR("Nothing here, either"); 
			exit(-1); 
		}
	}

	uint64_t added_cycle = pendingWriteRequests[address].front();
	latency = done_cycle - added_cycle;

	pendingWriteRequests[address].pop_front();
	cout << "Write Callback: 0x"<< std::hex << address << std::dec << " latency="<<latency<<"cycles ("<< done_cycle<< "->"<<added_cycle<<")"<<endl;
}

// simulate L1 cache
void Npusim::l1_cache_sim(){ //simulate the L1 cache hit and miss
    for (int i = 0; i < Mv_Fifo.size();) {
        Mv_Fifo_Item mv_fifo_item;
        mv_fifo_item = Mv_Fifo[i];
        if (Valid_NN_Frame[mv_fifo_item._ref_idx]) {
            Mv_Fifo.erase(Mv_Fifo.begin() + i); //pop the exact mv_fifo_item
            //begin motion compesation
            int idx = mv_fifo_item._ref_idx;
            int x = mv_fifo_item._dst_x;
            int y = mv_fifo_item._dst_y;
            if(x % 8 !=0){
                if(y % 8 !=0){
                    // across 4 block
                    int x0 = x/8 * 8;
                    int x1 = x0 + 8;
                    int y0 = y/8 * 8;
                    int y1 = y0 + 8;
                    Data_Info x0_info;
                    Data_Info x1_info;
                    Data_Info y0_info;
                    Data_Info y1_info;
                    x0_info = generate_cache_addr(idx, x0);
                    x1_info = generate_cache_addr(idx, x1);
                    y0_info = generate_cache_addr(idx, y0);
                    y1_info = generate_cache_addr(idx, y1); // generate aligned information
                    Dram_Info dram_info1(idx,x0,y0);
                    Dram_Info dram_info2(idx,x0,y1);
                    Dram_Info dram_info3(idx,x1,y0);
                    Dram_Info dram_info4(idx,x1,y1);// generate DRAM information
                    if(!MC_L1_Cache.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x0, y0);
                        //pending L2 request
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }

                    if(!MC_L1_Cache.check_cache_hit(x0_info,y1_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x0, y1);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }

                    if(!MC_L1_Cache.check_cache_hit(x1_info,y0_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x1, y0);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }

                    if(!MC_L1_Cache.check_cache_hit(x1_info,y1_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x1, y1);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                }
                else{
                    // horizontal: across 2 block
                    int x0 = x/8 * 8;
                    int x1 = x0 + 8;
                    Data_Info x0_info;
                    Data_Info x1_info;
                    Data_Info y0_info;
                    x0_info = generate_cache_addr(idx, x0);
                    x1_info = generate_cache_addr(idx, x1);
                    y0_info = generate_cache_addr(idx, y);
                    Dram_Info dram_info1(idx,x0,y);
                    Dram_Info dram_info2(idx,x1,y);// generate DRAM information
                    if(!MC_L1_Cache.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x0, y);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 

                    if(!MC_L1_Cache.check_cache_hit(x1_info,y0_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x1, y);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                }
            }
            else{
                if(y % 8 !=0){
                    // vertical: across 2 block
                    int y0 = y/8 * 8;
                    int y1 = y0 + 8;
                    Data_Info x0_info;
                    Data_Info y0_info;
                    Data_Info y1_info;
                    x0_info = generate_cache_addr(idx, x);
                    y0_info = generate_cache_addr(idx, y0);
                    y1_info = generate_cache_addr(idx, y1);
                    Dram_Info dram_info1(idx,x,y0);
                    Dram_Info dram_info2(idx,x,y1);
                    if(!MC_L1_Cache.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x, y0);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }

                    if(!MC_L1_Cache.check_cache_hit(x0_info,y1_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x, y1);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                }
                else{
                    // one block
                    Data_Info x0_info;
                    Data_Info y0_info;
                    x0_info = generate_cache_addr(idx, x);
                    y0_info = generate_cache_addr(idx, y);
                    Dram_Info dram_info1(idx,x,y);
                    if(!MC_L1_Cache.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x, y);
                        L2_Request temp_request(temp_mv_fifo_item,systimer);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                }
            }
            //end motion compesation
            MC_L1_Cache.update_frequency(); //update frequency
            MC_L2_Cache.update_frequency(); //use to update frequency
            break;
        }
        else {
            i++;
            continue;
        }
    }
}

// simulate L2 cache
void Npusim::l2_cache_sim(){ //simulate the L2 cache hit and miss
    //check L2 request pending
    for (int i = 0; i < Pending_L2_request_list.size();) {
        if (Pending_L2_request_list[i]._return_cycle == systimer) {
            //begin check L2 cache hit
            Mv_Fifo_Item temp_mv_fifo_item = Pending_L2_request_list[i]._mv_fifo_item;
            int idx = temp_mv_fifo_item._ref_idx;
            int x = temp_mv_fifo_item._dst_x;
            int y = temp_mv_fifo_item._dst_y;
            // cout<<"test: x = "<<x<<", y = "<<y<<endl;
            Data_Info x0_info;
            Data_Info y0_info;
            x0_info = generate_cache_addr(idx, x);
            y0_info = generate_cache_addr(idx, y);
            Dram_Info dram_info(idx,x,y);// generate DRAM information
            if(MC_L2_Cache.check_cache_hit(x0_info,y0_info)){ //L2 cache hit
                MC_L1_Cache.replace_data(x0_info,y0_info);
            }
            else{ 
                //L2 cache miss
                //pending L2 request list
                Request temp_request(temp_mv_fifo_item,dram_info);
                Pending_request_list.push_back(temp_request); // pending the corresponding request of the exact mv item
                //pending DRAM request list
                
                bool flag = false;
                for(int i = 0; i < Dram_Request_list.size(); i++){
                    if(Dram_Request_list[i]._Dram_addr.compare(temp_request._Dram_addr)==0){ 
                        // if addr is in the pending list, merge access request
                        flag = true;
                    }
                }
                if(flag == false){
                    // if addr is not in the pending list, add it and request DRAM
                    Dram_Request dram_request;
                    dram_request.init_DRAM_Request(temp_request._Dram_addr);
                    Dram_Request_list.push_back(dram_request);
                    
                    // dramsim2
                    // TODO: At the same cycle, dram can accept multiple requests, this is different from DramSim2 in design.
                    uint64_t dram_addr;
                    string hex_addr = bin2hex(temp_request._Dram_addr);
                    istringstream a(hex_addr.substr(2));
                    a >> hex >> dram_addr;
                    trans = new Transaction(DATA_READ, dram_addr, NULL);
                    // trans->address >>= THROW_AWAY_BITS;
                    // trans->address <<= THROW_AWAY_BITS;
                    if((*memorySystem).addTransaction(trans)){
                        add_pending(trans, systimer);
                        trans = NULL;
                    }
                }
            }
            //end check L2 cache hit
            Pending_L2_request_list.erase(Pending_L2_request_list.begin() + i);
        }
        else i++;
    }
    (*memorySystem).update();
}


void Npusim::controller(){
    int temp_large_NN = 0;
    int temp_decode = 0;
    int temp_small_NN = 0;
    while(!Decode_IP_frame_idx.empty()||!Decode_B_frame_idx.empty()){
        systimer++;    // system clock
        // process I/P frames
        if (!Decode_IP_frame_idx.empty()) {
            int frame_IP_idx = Decode_IP_frame_idx.front();   // current I/P frame
            FRAME_T frame_IP_type = Frame_type[frame_IP_idx]; // type of I/P frame: I_FRAME / P_FRAME
            if (Valid_Decode_Frame[frame_IP_idx]) {
                // I or P frame do nerual network
                temp_large_NN++;
                if (temp_large_NN > neural_network_sim(frame_IP_type)) {
                    // nn finished
                    cout<<"IP - frame: "<<frame_IP_idx<<" nn finished."<<endl; //test case
                    Valid_NN_Frame[frame_IP_idx] = 1;
                    Decode_IP_frame_idx.erase(Decode_IP_frame_idx.begin()); // pop the exact I or P frame idx
                    temp_large_NN = 0;
                }
            }
            else{ 
                // I and P frame need to be decoded, while B frame does not need
               temp_decode++;
               if(temp_decode>decoder_sim()){
                   Valid_Decode_Frame[frame_IP_idx] = 1;
                   temp_decode = 0;
               }
            }
        }

        // process B frames
        if (!Decode_B_frame_idx.empty()) {
            int frame_B_idx = Decode_B_frame_idx.front();   // current B frame
            FRAME_T frame_B_type = Frame_type[frame_B_idx]; // type of B frame: B_FRAME
            if (Valid_MC_Frame[frame_B_idx]) { 
                // B frame execute Neural network
                temp_small_NN++;
                if (temp_small_NN > neural_network_sim(frame_B_type)) {
                    // nn finished
                    Valid_NN_Frame[frame_B_idx] = 1;
                    Decode_B_frame_idx.erase(Decode_B_frame_idx.begin()); // pop the exact B frame idx
                    temp_small_NN = 0;
                }
            }
            // execute MC process, update Valid_MC_Frame
            // dram_sim();
            l2_cache_sim();
            
            l1_cache_sim();

            // judge if motion compensation(MC) is complete
            int temp_i = -1;
            int temp_j = -1;
            for (int i = 0; i < Mv_Fifo.size(); i++) {
                if (frame_B_idx != Mv_Fifo[i]._b_idx) {
                    continue;
                }
                else {
                    temp_i = i;
                    break;
                }
            }
            for (int j = 0; j < Pending_request_list.size(); j++) {
                if (frame_B_idx != Pending_request_list[j]._mv_fifo_item._b_idx) {
                    continue;
                }
                else {
                    temp_j = j;
                    break;
                }
            }
            if (temp_i == -1 && temp_j == -1) {
                Valid_MC_Frame[frame_B_idx] = 1;
            }
        }
    }
}

// init test case
void Npusim::sysini(){
    // init Decode_frame_idx
    Decode_frame_idx.push_back(0);
    Decode_frame_idx.push_back(3);
    Decode_frame_idx.push_back(2);
    Decode_frame_idx.push_back(1);

    // init Decode_IP_frame_idx
    Decode_IP_frame_idx.push_back(0);
    Decode_IP_frame_idx.push_back(3);

    // init Decode_B_frame_idx
    Decode_B_frame_idx.push_back(2);
    Decode_B_frame_idx.push_back(1);

    // init Frame_type
    Frame_type.push_back(I_FRAME);
    Frame_type.push_back(B_FRAME), Frame_type.push_back(B_FRAME);
    Frame_type.push_back(P_FRAME);

    // init Mv_fifo
    Mv_Fifo_Item tmpobj_0(2,8,8,0,0,0,8,2);
    Mv_Fifo_Item tmpobj_1(2,8,8,0,0,3,1,0);
    Mv_Fifo_Item tmpobj_2(2,8,8,0,8,0,0,3);
    Mv_Fifo_Item tmpobj_3(2,8,8,0,8,3,3,8);
    Mv_Fifo_Item tmpobj_4(2,8,8,0,16,3,4,7);
    Mv_Fifo_Item tmpobj_5(2,8,8,0,16,0,2,8);
    Mv_Fifo_Item tmpobj_6(2,8,8,0,24,0,0,6);
    Mv_Fifo_Item tmpobj_7(2,8,8,0,24,3,6,7);
    Mv_Fifo_Item tmpobj_8(2,8,8,8,0,0,2,5);
    Mv_Fifo_Item tmpobj_9(2,8,8,8,0,3,2,0);
    Mv_Fifo_Item tmpobj_10(2,8,8,8,8,0,4,6);
    Mv_Fifo_Item tmpobj_11(2,8,8,8,8,3,5,3);

    Mv_Fifo.push_back(tmpobj_0);
    Mv_Fifo.push_back(tmpobj_1);
    Mv_Fifo.push_back(tmpobj_2);
    Mv_Fifo.push_back(tmpobj_3);
    Mv_Fifo.push_back(tmpobj_4);
    Mv_Fifo.push_back(tmpobj_5);
    Mv_Fifo.push_back(tmpobj_6);
    Mv_Fifo.push_back(tmpobj_7);
    Mv_Fifo.push_back(tmpobj_8);
    Mv_Fifo.push_back(tmpobj_9);
    Mv_Fifo.push_back(tmpobj_10);
    Mv_Fifo.push_back(tmpobj_11);

    tmpobj_0.init_mv_fifo_item(2,8,8,8,16,0,6,3);
    tmpobj_1.init_mv_fifo_item(2,8,8,8,16,3,5,1);
    tmpobj_2.init_mv_fifo_item(2,8,8,8,24,0,8,4);
    tmpobj_3.init_mv_fifo_item(2,8,8,8,24,3,5,4);
    tmpobj_4.init_mv_fifo_item(1,8,8,0,0,0,1,7);
    tmpobj_5.init_mv_fifo_item(1,8,8,0,0,2,6,8);
    tmpobj_6.init_mv_fifo_item(1,8,8,0,8,3,3,7);
    tmpobj_7.init_mv_fifo_item(1,8,8,0,8,0,4,0);
    tmpobj_8.init_mv_fifo_item(1,8,8,0,16,0,0,5);
    tmpobj_9.init_mv_fifo_item(1,8,8,0,16,2,8,6);
    tmpobj_10.init_mv_fifo_item(1,8,8,0,24,0,1,3);
    tmpobj_11.init_mv_fifo_item(1,8,8,0,24,3,2,7);

    Mv_Fifo.push_back(tmpobj_0);
    Mv_Fifo.push_back(tmpobj_1);
    Mv_Fifo.push_back(tmpobj_2);
    Mv_Fifo.push_back(tmpobj_3);
    Mv_Fifo.push_back(tmpobj_4);
    Mv_Fifo.push_back(tmpobj_5);
    Mv_Fifo.push_back(tmpobj_6);
    Mv_Fifo.push_back(tmpobj_7);
    Mv_Fifo.push_back(tmpobj_8);
    Mv_Fifo.push_back(tmpobj_9);
    Mv_Fifo.push_back(tmpobj_10);
    Mv_Fifo.push_back(tmpobj_11);

    tmpobj_0.init_mv_fifo_item(1,8,8,8,0,2,6,2);
    tmpobj_1.init_mv_fifo_item(1,8,8,8,0,3,0,1);
    tmpobj_2.init_mv_fifo_item(1,8,8,8,8,3,5,6);
    tmpobj_3.init_mv_fifo_item(1,8,8,8,8,2,3,6);
    tmpobj_4.init_mv_fifo_item(1,8,8,8,16,0,2,8);
    tmpobj_5.init_mv_fifo_item(1,8,8,8,16,3,0,5);
    tmpobj_6.init_mv_fifo_item(1,8,8,8,24,0,8,8);
    tmpobj_7.init_mv_fifo_item(1,8,8,8,24,2,0,8);
    tmpobj_8.init_mv_fifo_item(1,8,8,16,0,0,4,5);
    tmpobj_9.init_mv_fifo_item(1,8,8,16,0,2,6,6);
    tmpobj_10.init_mv_fifo_item(1,8,8,16,8,0,8,0);
    tmpobj_11.init_mv_fifo_item(1,8,8,16,8,3,8,8);

    Mv_Fifo.push_back(tmpobj_0);
    Mv_Fifo.push_back(tmpobj_1);
    Mv_Fifo.push_back(tmpobj_2);
    Mv_Fifo.push_back(tmpobj_3);
    Mv_Fifo.push_back(tmpobj_4);
    Mv_Fifo.push_back(tmpobj_5);
    Mv_Fifo.push_back(tmpobj_6);
    Mv_Fifo.push_back(tmpobj_7);
    Mv_Fifo.push_back(tmpobj_8);
    Mv_Fifo.push_back(tmpobj_9);
    Mv_Fifo.push_back(tmpobj_10);
    Mv_Fifo.push_back(tmpobj_11);
}



int main(){
    Npusim npu(4);
    npu.sysini();
    npu.controller();

    return 0;
}