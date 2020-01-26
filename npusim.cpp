#include "npusim.h"

// init Npusim
Npusim::Npusim(){
    systimer = 0;
    MC_L1_Cache.init_mc_cache(2, 2, 1, 1, 4, 4, 8, 8, 0);
    MC_L2_Cache.init_mc_cache(2, 2, 1, 1, 8, 8, 8, 8, 1);
    systemIniFilename = "ini/system.ini";
    deviceIniFilename = "ini/DDR3_micron_64M_8B_x4_sg15.ini";
    memorySystem = new MultiChannelMemorySystem(deviceIniFilename, systemIniFilename, "", "", 2048, NULL, NULL);
    memorySystem->setCPUClockSpeed(0); 	// set the frequency ratio to 1:1
    read_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::read_complete);
    write_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::write_complete);
	memorySystem->RegisterCallbacks(read_cb, write_cb, NULL);
    trans = NULL;
}

Npusim::Npusim(MC_Cache _l1_cache_config, MC_Cache _l2_cache_config){ //input ini.file
    systimer = 0;
    MC_L1_Cache = _l1_cache_config;
    MC_L2_Cache = _l2_cache_config;
    systemIniFilename = "ini/system.ini";
    deviceIniFilename = "ini/DDR3_micron_64M_8B_x4_sg15.ini";
    memorySystem = new MultiChannelMemorySystem(deviceIniFilename, systemIniFilename, "", "", 2048, NULL, NULL);
    memorySystem->setCPUClockSpeed(0); 	// set the frequency ratio to 1:1
    read_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::read_complete);
    write_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::write_complete);
	memorySystem->RegisterCallbacks(read_cb, write_cb, NULL);
    trans = NULL;
}

// path - common path(e.g. "/home/liuxueyuan/npusim"), b/p_fname - b/p file path(e.g. "b/test" or "p/test") 
void Npusim::load_ibp_label(string path, string b_fname, string p_fname, string i_fname=""){
    string b_path = path + '/' + b_fname;
    string p_path = path + '/' + p_fname;
    ifstream b_inFile(b_path.c_str(), ios::in);
    ifstream p_inFile(p_path.c_str(), ios::in);
    string b_elem, p_elem;
    int cntb = 0, cntp = 0, cnti = 0; // calc num of b/p/i frames
    while (getline(b_inFile, b_elem)){
        frame_type_maptab[atoi(b_elem.c_str())-1] = B_FRAME;
        cntb++;
    }
    if(!i_fname.empty()){
        string i_path = path + '/' + i_fname;
        ifstream i_inFile(i_path.c_str(), ios::in);
        string i_elem;
        while(getline(i_inFile, i_elem)){
            frame_type_maptab[atoi(i_elem.c_str())-1] = I_FRAME;
            cnti++;
        }
        while(getline(p_inFile, p_elem)){
            frame_type_maptab[atoi(p_elem.c_str())-1] = P_FRAME;
            cntp++;
        }
    }
    else{
        while(getline(p_inFile, p_elem)){
            if(cntp > 0){
                frame_type_maptab[atoi(p_elem.c_str())-1] = P_FRAME;
                cntp++;
            }
            else{
                frame_type_maptab[0] = I_FRAME;
                cnti++;
            }
        }
    }
    num_frame = cntb + cntp + cnti;
    Valid_NN_Frame.assign(num_frame, 0);
    Valid_MC_Frame.assign(num_frame, 0);
    Valid_Decode_Frame.assign(num_frame, 0);
}

// load b-frame motion vectors(load mv_table from ffmpeg)
// remove the case of negative value
// split block size to 8*8
void Npusim::load_mvs(string filename){
    ifstream inFile(filename.c_str(), ios::in);
    string lineStr;
    int cnt=0;
    // while (getline(inFile, lineStr)&&cnt<5000)
    while (getline(inFile, lineStr))
    {
        cnt++;
        stringstream ss(lineStr);
        string str;
        vector<int> lineArray;
        while (getline(ss, str, ',')) {
            lineArray.push_back(atoi(str.c_str()));
        }
        if (lineArray[6] < 0 || lineArray[7] < 0) continue;  // exclude the case of negative value
        // 0-idx, 1-ref_idx, 2-width, 3-height, 4-src_x, 5-src_y, 6-dest_x, 7-dest_y
    
        for (int i = lineArray[2] - 8; i >= 0; i -= 8) {
            for (int j = lineArray[3] - 8; j >= 0; j -= 8) {
                Mv_Fifo_Item temp_item(lineArray[0], 8, 8, lineArray[4] + i, lineArray[5] + j, lineArray[1], lineArray[6] + i, lineArray[7] + j);
                Mv_Fifo.push_back(temp_item);
            }
        }
    }
}

// get decode order
void Npusim::decode_order(){
    // generate_DAG
    for(int i = 0; i < MAX_SIZE_OF_FTMTAB; i++){
        graph.visited[i] = 0;
        for(int j = 0; j < MAX_SIZE_OF_FTMTAB; j++){
            graph.arc[i][j] = 0;
        }
    }
    for(int i = 0; i < Mv_Fifo.size(); i++){
        int b_idx = Mv_Fifo[i]._b_idx;
        int ref_idx = Mv_Fifo[i]._ref_idx;
        graph.arc[b_idx][ref_idx] = 1;
    }

    bool flag;
    int t = 0;
    for(int i = 0; i < num_frame; i++){
        if(graph.visited[i] == 0){
            flag = false;
            for(int j = 0; j < num_frame; j++){
                if(graph.arc[i][j] == 1){
                    flag = true;
                    break;
                }
            }
            if(flag){
                continue;
            }
            else{
                order[t] = i;
                if(frame_type_maptab[i] == B_FRAME){
                    Decode_B_frame_idx.push_back(i);
                }
                else{
                    Decode_IP_frame_idx.push_back(i);
                }
                for(int h = 0; h < num_frame; h++){
                    graph.arc[h][i] = 0;
                }
                graph.visited[i] = 1;
                t++;
                i = 0;
            }
        }
    }
    for(int idx=0; idx<num_frame; idx++){
        reorder[order[idx]] = idx;
    }
    for(int i=0; i<Mv_Fifo.size(); ++i){
        Mv_Fifo[i].ordmap = reorder[Mv_Fifo[i]._b_idx];
    }
}

bool cmp(const Mv_Fifo_Item& a, const Mv_Fifo_Item& b) {
    if(a.ordmap == b.ordmap){
        if (a._src_x != b._src_x)
            return a._src_x < b._src_x;
        return a._src_y < b._src_y;
    }
    return a.ordmap < b.ordmap;
}

// sort mv_fifo
void Npusim::sort_mvs(){
    vector<Mv_Fifo_Item>::iterator it = Mv_Fifo.begin();
    // remove I/P frame from Mv_Fifo
    for(int i=0; i<Mv_Fifo.size();){
        if(frame_type_maptab[Mv_Fifo[i]._b_idx] != B_FRAME){
            Mv_Fifo.erase(it+i);
        }
        else i++;
    }
    // sort B-frame in Mv_Fifo
    sort(Mv_Fifo.begin(), Mv_Fifo.end(), cmp);
}

// judge if a frame is bidirectional prediction frame
void Npusim::judge_bid_pred(){
    for(int i=1, pre_idx=0; i<Mv_Fifo.size(); ++i, ++pre_idx){
        if(Mv_Fifo[pre_idx]._b_idx == Mv_Fifo[i]._b_idx && Mv_Fifo[pre_idx]._src_x == Mv_Fifo[i]._src_x && Mv_Fifo[pre_idx]._src_y == Mv_Fifo[i]._src_y && Mv_Fifo[pre_idx]._ref_idx != Mv_Fifo[pre_idx]._ref_idx){
            Mv_Fifo[pre_idx].isBidPredFrame = true;
            Mv_Fifo[i].isBidPredFrame = true;
        }
    }
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
	cout << "Read Callback:  0x"<< std::hex << std::setw(8) << std::setfill('0') << address << std::dec << " latency="<< latency <<"cycles ("<< added_cycle << "->"<< done_cycle <<")"<<endl;
    
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
                    tmp_x0_info = MC_L1_Cache.generate_cache_addr(tmp_idx, tmp_x, 0);
                    tmp_y0_info = MC_L1_Cache.generate_cache_addr(tmp_idx, tmp_y, 1);
                    MC_L1_Cache.replace_data(tmp_x0_info,tmp_y0_info);
                    tmp_x0_info = MC_L2_Cache.generate_cache_addr(tmp_idx, tmp_x, 0);
                    tmp_y0_info = MC_L2_Cache.generate_cache_addr(tmp_idx, tmp_y, 1);
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
        // cout<<"l1 cache"<<endl;
        Mv_Fifo_Item mv_fifo_item;
        mv_fifo_item = Mv_Fifo[i];
        // cout<<"cycle: "<<systimer<<", Mv_item: ("<<mv_fifo_item._b_idx<<", "<<mv_fifo_item._ref_idx<<", "<<mv_fifo_item._src_x<<", "<<mv_fifo_item._src_y<<", "<<mv_fifo_item._dst_x<<", "<<mv_fifo_item._dst_y<<")"<<endl; //test
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
                    x0_info = MC_L1_Cache.generate_cache_addr(idx, x0, 0);
                    x1_info = MC_L1_Cache.generate_cache_addr(idx, x1, 0);
                    y0_info = MC_L1_Cache.generate_cache_addr(idx, y0, 1);
                    y1_info = MC_L1_Cache.generate_cache_addr(idx, y1, 1); // generate aligned information
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
                    x0_info = MC_L1_Cache.generate_cache_addr(idx, x0, 0);
                    x1_info = MC_L1_Cache.generate_cache_addr(idx, x1, 0);
                    y0_info = MC_L1_Cache.generate_cache_addr(idx, y, 1);
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
                    x0_info = MC_L1_Cache.generate_cache_addr(idx, x, 0);
                    y0_info = MC_L1_Cache.generate_cache_addr(idx, y0, 1);
                    y1_info = MC_L1_Cache.generate_cache_addr(idx, y1, 1);
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
                    x0_info = MC_L1_Cache.generate_cache_addr(idx, x, 0);
                    y0_info = MC_L1_Cache.generate_cache_addr(idx, y, 1);
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
        // cout<<"l2 cache"<<endl;//test
        if (Pending_L2_request_list[i]._return_cycle == systimer) {
            //begin check L2 cache hit
            Mv_Fifo_Item temp_mv_fifo_item = Pending_L2_request_list[i]._mv_fifo_item;
            int idx = temp_mv_fifo_item._ref_idx;
            int x = temp_mv_fifo_item._dst_x;
            int y = temp_mv_fifo_item._dst_y;
            // cout<<"test: x = "<<x<<", y = "<<y<<endl;
            Data_Info x0_info;
            Data_Info y0_info;
            x0_info = MC_L2_Cache.generate_cache_addr(idx, x, 0);
            y0_info = MC_L2_Cache.generate_cache_addr(idx, y, 1);
            Dram_Info dram_info(idx,x,y);// generate DRAM information
            if(MC_L2_Cache.check_cache_hit(x0_info,y0_info)){ //L2 cache hit

                MC_L1_Cache.replace_data(x0_info,y0_info);
            }
            else{ 
                //L2 cache miss
                //pending L2 request list
                // cout<<"1-capacity: "<<Pending_request_list.capacity()<<", size: "<<Pending_request_list.size()<<endl;
                // cout<<"1.5 - "<<temp_mv_fifo_item._b_idx<<" "<<dram_info._idx<<endl;
                Request temp_request(temp_mv_fifo_item,dram_info);
                // cout<<"hahahahahaha"<<endl;
                // cout<<"2-capacity: "<<Pending_request_list.capacity()<<", size: "<<Pending_request_list.size()<<endl;
                Pending_request_list.push_back(temp_request); // pending the corresponding request of the exact mv item
                //pending DRAM request list
                // cout<<"ccoooooout! systimer: "<<systimer<<endl;
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
                    // cout<<"request dramsim"<<endl;
                    uint64_t dram_addr;
                    string hex_addr = bin2hex(temp_request._Dram_addr);
                    istringstream a(hex_addr.substr(2));
                    a >> hex >> dram_addr;
                    trans = new Transaction(DATA_READ, dram_addr, NULL);
                    // if(!trans){
                    //     cout<<"********* EMPTY ***************"<<endl;
                    // }
                    // trans->address >>= THROW_AWAY_BITS;
                    // trans->address <<= THROW_AWAY_BITS;
                    if((*memorySystem).addTransaction(trans)){
                        // cout<<"addTransaction!!!!! systimer: "<<systimer<<endl;
                        add_pending(trans, systimer);
                        trans = NULL;
                    }
                }
            }
            //end check L2 cache hit
            Pending_L2_request_list.erase(Pending_L2_request_list.begin() + i);
            // cout<<"erase!!!!! systimer: "<<systimer<<endl;
            break; //each cycle only process one request
        }
        else i++;
    }
    (*memorySystem).update();
    // cout<<"update!!!!! systimer: "<<systimer<<endl;
}

void Npusim::controller(){
    int temp_large_NN = 0;
    int temp_decode = 0;
    int temp_small_NN = 0;
    while(!Decode_IP_frame_idx.empty()||!Decode_B_frame_idx.empty()){
        systimer++;    // system clock
        // cout<<"curcycle in controller: "<<systimer<<endl;
        // if(systimer >= 375){
        //     cout<<"time is :"<<systimer<<endl;
        // }
        // cout<<"controller"<<endl;  //test
        // process I/P frames
        if (!Decode_IP_frame_idx.empty()) {
            int frame_IP_idx = Decode_IP_frame_idx.front();   // current I/P frame
            FRAME_T frame_IP_type = frame_type_maptab[frame_IP_idx]; // type of I/P frame: I_FRAME / P_FRAME
            if (Valid_Decode_Frame[frame_IP_idx]) {
                // I or P frame do nerual network
                temp_large_NN++;
                if (temp_large_NN > neural_network_sim(frame_IP_type)) {
                    // nn finished
                    cout<<frame_IP_idx<<" nn finished"<<endl;
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
            FRAME_T frame_B_type = frame_type_maptab[frame_B_idx]; // type of B frame: B_FRAME
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

// for test
void Npusim::init(){
    num_frame=3;
    Valid_NN_Frame.assign(num_frame, 0);
    Valid_MC_Frame.assign(num_frame, 0);
    Valid_Decode_Frame.assign(num_frame, 0);

    Decode_B_frame_idx.push_back(1);
    Decode_B_frame_idx.push_back(2);
    Decode_IP_frame_idx.push_back(0);
    frame_type_maptab[0]=I_FRAME;
    frame_type_maptab[1]=B_FRAME;
    frame_type_maptab[2]=B_FRAME;
    Mv_Fifo_Item item(1,0,8,8,0,0,8,14);
    Mv_Fifo.push_back(item);
    for(int i=0;i<10000;++i){
        Mv_Fifo_Item item(2,1,8,8,i*8,(i+1)*8,i,3+i);
        Mv_Fifo.push_back(item);
    }
    
    // item.init_mv_fifo_item(2,1,8,8,16,8,2,13);
    // Mv_Fifo.push_back(item);
    // item.init_mv_fifo_item(2,1,8,8,8,8,2,13);
    // Mv_Fifo.push_back(item);
    // item.init_mv_fifo_item(2,1,8,8,24,16,2,13);
    // Mv_Fifo.push_back(item);
    // item.init_mv_fifo_item(2,1,8,8,8,0,2,13);
    // Mv_Fifo.push_back(item);    
}

// for test
void Npusim::test(){
    cout<<"Mv_Fifo.size = "<<Mv_Fifo.size()<<endl;
    for(int i=0;i<Mv_Fifo.size();++i){
        cout<<Mv_Fifo[i]._b_idx<<", "<<Mv_Fifo[i]._width<<", "<<Mv_Fifo[i]._height<<", ";
        cout<<Mv_Fifo[i]._src_x<<", "<<Mv_Fifo[i]._src_y<<", "<<Mv_Fifo[i]._ref_idx<<", ";
        cout<<Mv_Fifo[i]._dst_x<<", "<<Mv_Fifo[i]._dst_y<<endl;
    }
}

int main(){
    Npusim npu;
    npu.load_ibp_label("./idx","b/bear","p/bear");
    npu.load_mvs("./mvs/bear.csv");
    
    npu.decode_order();
    
    npu.sort_mvs();
    // npu.test();
    // cout<<"^^^^^^^^^^^^^^^^^^^^^"<<endl;
    npu.judge_bid_pred();
    // npu.test();
    // npu.init();

    npu.controller();

    return 0;
}