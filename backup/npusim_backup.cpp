#include "npusim.h"

const int frameWidth = 854, frameHeight = 480;
int l1HitCnt = 0, l2HitCnt = 0, mergeAccessCnt = 0;

//test
bool isReadcallback = false;

// init Npusim
Npusim::Npusim(){
    systimer = 0;
    maxid_dramReqList = 0;
    isLargeNetStart = false, isSmallNetStart = false;
    isLargeNetOk = false, isSmallNetOk = false;
    isLnnLayerOk = false, isSnnLayerOk = false;
    cacheIniFilename  = "ini/cache.ini";
    systemIniFilename = "ini/system.ini";
    deviceIniFilename = "ini/DDR3_micron_64M_8B_x4_sg15.ini";
    MC_L1_Cache.init(cacheIniFilename, _L1_CACHE);
    MC_L2_Cache.init(cacheIniFilename, _L2_CACHE);
    memorySystem = new MultiChannelMemorySystem(deviceIniFilename, systemIniFilename, "", "", 2048, NULL, NULL);
    memorySystem->setCPUClockSpeed(0); 	// set the frequency ratio to 1:1
    read_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::read_complete);
    write_cb = new Callback<Npusim, void, unsigned, uint64_t, uint64_t>(this, &Npusim::write_complete);
	memorySystem->RegisterCallbacks(read_cb, write_cb, NULL);
    trans = NULL;
    mvFifoTraversalLabel = true;
    largeNet.init(LNN_T);    // init large npu
    smallNet.init(SNN_T);    // init small npu
}

// path - common path(e.g. "/home/liuxueyuan/npusim"), b/p_fname - b/p file path(e.g. "b/test" or "p/test") 
void Npusim::load_ibp_label(string path, string b_fname, string p_fname, string i_fname){
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
        int cnt = 0;
        while(getline(p_inFile, p_elem)){
            if(cnt > 0){
                frame_type_maptab[atoi(p_elem.c_str())-1] = P_FRAME;
                cntp++;
            }
            else{
                frame_type_maptab[0] = I_FRAME;
                cnti++;
            }
            cnt++;
        }
    }
    numFrames = cntb + cntp + cnti;
    frameValidBuffer.assign(numFrames, 0);
    frameMcValid.assign(numFrames, 0);
}

// load b-frame motion vectors(load mv_table from ffmpeg)
// remove the case of negative value
// split block size to 8*8
void Npusim::load_mvs(string filename){
    ifstream inFile(filename.c_str(), ios::in);
    string lineStr;
    while (getline(inFile, lineStr))
    {
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
    // DAG according to the decode order
    int order[MAX_SIZE_OF_FTMTAB];   // order[idx] = ord
    int reorder[MAX_SIZE_OF_FTMTAB]; // reverse order: reorder[ord] = idx
    Graph graph;

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
    for(int i = 0; i < numFrames; i++){
        if(graph.visited[i] == 0){
            flag = false;
            for(int j = 0; j < numFrames; j++){
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
                    Decode_IP_frame_idx.push(i);
                }
                for(int h = 0; h < numFrames; h++){
                    graph.arc[h][i] = 0;
                }
                graph.visited[i] = 1;
                t++;
                i = 0;
            }
        }
    }
    for(int idx=0; idx<numFrames; idx++){
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
    for(int i=0; i<Mv_Fifo.size()-1; ++i){
        if(Mv_Fifo[i]._b_idx == Mv_Fifo[i+1]._b_idx && Mv_Fifo[i]._src_x == Mv_Fifo[i+1]._src_x && Mv_Fifo[i]._src_y == Mv_Fifo[i+1]._src_y){
            Mv_Fifo[i].isBidPredFrame = true, Mv_Fifo[i+1].isBidPredFrame = true;
        }
    }
    map<int, int> bpfInBframe;  // frame_B_idx, bidPredFrame_nums
    for(vector<Mv_Fifo_Item>::iterator it = Mv_Fifo.begin(); it != Mv_Fifo.end(); it++){
        blkInBframe[it->_b_idx]++;
        if(it->isBidPredFrame){
            bpfInBframe[it->_b_idx]++;
        }
    }
    for(map<int, int>::iterator it = blkInBframe.begin(); it != blkInBframe.end(); it++){
        it->second -= (bpfInBframe[it->first]/2);
    }
}

void Npusim::getNpuBaseAddr(){
    Dram_Info lnnWInfo(105, 0, 0);
    string s_lnnWeightStartAddr = bin2hex(generate_dram_addr(lnnWInfo)).substr(2);
    istringstream lnnwiss(s_lnnWeightStartAddr);
    lnnwiss >> hex >> lnnWeightStartAddr;
    double lnnWeightSize = largeNet.weightAddrOffset.back() + largeNet.netArch.back().weights;
    int offset = ceil(1.0 * lnnWeightSize / (1024 * 1024* 16));
    Dram_Info snnWInfo(105 + offset, 0, 0);
    string s_snnWeightStartAddr = bin2hex(generate_dram_addr(snnWInfo)).substr(2);
    istringstream snnwiss(s_snnWeightStartAddr);
    snnwiss >> hex >> snnWeightStartAddr;
    double snnWeightSize = smallNet.weightAddrOffset.back() + smallNet.netArch.back().weights;
    offset += ceil(1.0 * snnWeightSize / (1024 * 1024 * 16));
    Dram_Info lnnRwInfo(105 + offset, 0, 0);
    string s_lnnRwStartAddr = bin2hex(generate_dram_addr(lnnRwInfo)).substr(2);
    istringstream lnnrwiss(s_lnnRwStartAddr);
    lnnrwiss >> hex >> lnnRWStartAddr;
    snnRWStartAddr = lnnRWStartAddr + largeNet.outputAddrOffset.back() + largeNet.netArch.back().outputs;
    ipDecodedFramesAddr = snnRWStartAddr + smallNet.outputAddrOffset.back() + smallNet.netArch.back().outputs;
    bMcFramesAddr = ipDecodedFramesAddr + (frameWidth * frameHeight * 3) * Decode_IP_frame_idx.size();
}

void Npusim::init(string ibpPath, string bFname, string pFname, string mvsFname, string iFname=""){
    load_ibp_label(ibpPath, bFname, pFname, iFname);
    load_mvs(mvsFname);
    getNpuBaseAddr();
    decode_order();
    sort_mvs();
    judge_bid_pred();
}

// simulate the neural network
int Npusim::neuralnet_read_sim(NeuralNetwork net, int fidx, int curlayer){
    NnType netType = net.getNetType();
    // readsim include load weights and load inputs
    int wTransNum = net.calcTransPerLayer(curlayer, READ_WEIGHTS);
    uint64_t wAddr = netType == LNN_T ? lnnWeightStartAddr + net.weightAddrOffset[curlayer] : snnWeightStartAddr + net.weightAddrOffset[curlayer];
    for(int i = 0; i < wTransNum; i++){
        wAddr += 64;
        trans = new Transaction(DATA_READ, wAddr, NULL);
        if((*memorySystem).addTransaction(false, wAddr)){
            add_pending(trans, systimer);
            if(netType == LNN_T){
                pendingLnnReadPerLayer.push_back(wAddr);
            }
            else{
                pendingSnnReadPerLayer.push_back(wAddr);
            }
            trans = NULL;
        }
    }
    int iTransNum = net.calcTransPerLayer(curlayer, READ_INPUTS);
    uint64_t iAddr = netType == LNN_T ? lnnRWStartAddr + net.outputAddrOffset[net.netArch[curlayer].dependency] : snnRWStartAddr + net.outputAddrOffset[net.netArch[curlayer].dependency];
    
    for(int i = 0; i <= iTransNum; ++i){
        iAddr += 64;
        trans = new Transaction(DATA_READ, iAddr, NULL);
        if((*memorySystem).addTransaction(false, iAddr)){
            add_pending(trans, systimer);
            if(netType == LNN_T){
                pendingLnnReadPerLayer.push_back(wAddr);
            }
            else{
                pendingSnnReadPerLayer.push_back(wAddr);
            }
            trans = NULL;
        }
    }
}

int Npusim::neuralnet_write_sim(NeuralNetwork net, int fidx, int curlayer){
    NnType netType = net.getNetType();
    int rTransNum = net.calcTransPerLayer(curlayer, WRITE_OUTPUTS);
    uint64_t oAddr = netType == LNN_T ? lnnRWStartAddr + net.outputAddrOffset[curlayer] : snnRWStartAddr + net.outputAddrOffset[curlayer];
    for(int i = 0; i <= rTransNum; ++i){
        oAddr += 64;
        trans = new Transaction(DATA_WRITE, oAddr, NULL);
        if((*memorySystem).addTransaction(true, oAddr)){
            add_pending(trans, systimer);
            if(netType == LNN_T)
                pendingLnnWritePerFrame.push_back(oAddr);
            else pendingSnnWritePerFrame.push_back(oAddr);
            trans = NULL;
        }
    }
}

// simulate hevc decoder
int Npusim::decoder_sim(){
    // int frequency = 3; //300MHz
    // int FPS = 6; //60fps
    // int cycle = 0.5 * pow(10,7);
    int cycle = 1;
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
    isReadcallback = true; //test
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

    ostringstream rb;
    rb << std::setw(8) << std::setfill('0') << hex << address;
    string addrStr = "0x" + rb.str();
	cout << "Read Callback: " << addrStr << std::dec << " latency="<< latency <<"cycles ("<< added_cycle << "->"<< done_cycle <<")"<<endl;

    if((address >= snnRWStartAddr && address < ipDecodedFramesAddr) || (address >= snnWeightStartAddr && address < lnnRWStartAddr) || (address >= bMcFramesAddr)){
        // small net read callback
        for(list<uint64_t>::iterator it = pendingSnnReadPerLayer.begin(); it != pendingSnnReadPerLayer.end(); it++){
            if(*it == address){
                it = pendingSnnReadPerLayer.erase(it);
                if(pendingSnnReadPerLayer.empty()){
                    isSnnLayerOk = true;
                }
                break;
            }
        }
    }
    else if((address >= lnnRWStartAddr && address < snnRWStartAddr) || (address >= lnnWeightStartAddr && address < snnWeightStartAddr) || (address >= ipDecodedFramesAddr && address < bMcFramesAddr)){
        // large net read callback
        for(list<uint64_t>::iterator it = pendingLnnReadPerLayer.begin(); it != pendingLnnReadPerLayer.end(); it++){
            if(*it == address){
                it = pendingLnnReadPerLayer.erase(it);
                if(pendingLnnReadPerLayer.empty()){
                    isLnnLayerOk = true;
                }
                break;
            }
        }
    }
    else{
        for(int i=0; i<Dram_request_list.size();){
            if(Dram_request_list[i].first == addrStr){
                int p = Dram_request_list[i].second;
                int tmp_idx, tmp_x, tmp_y;
                for(int j=0; j<Pending_request_list.size();){
                    vector<pair<Mv_Fifo_Item, int> >::iterator it = Pending_request_list[j]._mv_fifo_items.begin();
                    while(it!=Pending_request_list[j]._mv_fifo_items.end()){
                        if(it->second == p){
                            tmp_idx = it->first._ref_idx;
                            tmp_x = it->first._dst_x, tmp_y = it->first._dst_y;
                            it = Pending_request_list[j]._mv_fifo_items.erase(it);
                            Pending_request_list[j]._remain_num--;
                            break;
                        }
                        else it++;
                    }
                    if(Pending_request_list[j]._remain_num==0){
                        q_shifter.push(make_pair(Pending_request_list[j]._ori_mv_item, systimer+SHIFT_CYCLE));
                        Pending_request_list.erase(Pending_request_list.begin()+j);
                    }
                    else j++;
                }
                for(int cnt=0, x0=tmp_x, y0=tmp_y; cnt<4; cnt++){
                    Data_Info xinfo, yinfo;
                    xinfo = MC_L1_Cache.generate_cache_addr(tmp_idx, x0, 0);
                    yinfo = MC_L1_Cache.generate_cache_addr(tmp_idx, y0, 1);
                    MC_L1_Cache.replace_data(xinfo, yinfo);
                    xinfo = MC_L2_Cache.generate_cache_addr(tmp_idx, x0, 0);
                    yinfo = MC_L2_Cache.generate_cache_addr(tmp_idx, y0, 1);
                    MC_L2_Cache.replace_data(xinfo, yinfo);

                    if(x0 + 8 >= frameWidth+10){
                        x0 = 0, y0 += 8;
                    }
                    else x0 += 8;
                }
                Dram_request_list.erase(Dram_request_list.begin()+i);
                break;
            }
            else i++;
        }
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

    ostringstream wb;
    wb << std::setw(8) << std::setfill('0') << hex << address;
    string addrStr = "0x" + wb.str();
	cout << "Write Callback: "<< addrStr << " latency="<<latency<<"cycles ("<< added_cycle << "->"<<done_cycle<<")"<<endl;

    if(address >= snnRWStartAddr){
        // small npu write callback
        for(list<uint64_t>::iterator it = pendingSnnWritePerFrame.begin(); it != pendingSnnWritePerFrame.end(); it++){
            if(*it == address){
                it = pendingSnnWritePerFrame.erase(it);
                if(pendingSnnWritePerFrame.empty()){
                    isSmallNetOk = true;
                }
                break;
            }
        }
    }
    else if(address >= lnnRWStartAddr){
        // large npu write callback.
        for(list<uint64_t>::iterator it = pendingLnnWritePerFrame.begin(); it != pendingLnnWritePerFrame.end(); it++){
            if(*it == address){
                it = pendingLnnWritePerFrame.erase(it);
                if(pendingLnnWritePerFrame.empty()){
                    isLargeNetOk = true;
                }
                break;
            }
        }
    }
    else{
        // cache write request callback.
        // judge if motion compensation(MC) is complete
        for(map<int, list<uint64_t> >::iterator it = pendingWriteReqList.begin(); it != pendingWriteReqList.end();){
            list<uint64_t>::iterator it_addr = find(it->second.begin(), it->second.end(), address);
            if(it_addr == it->second.end()){
                // can not find in cur_bidx
                it++;
            }
            else{
                it_addr = it->second.erase(it_addr);
                if(it->second.size() == 0){
                    // All blocks in a I/B/P frame write callback
                    frameValidBuffer[it->first] = 1;
                    mvFifoTraversalLabel = true;
                    pendingWriteReqList.erase(it);
                }
                break;
            }
        }
    }
}

// simulate L1 cache
void Npusim::l1_cache_sim(){
    bool isWork = false;
    if(mvFifoTraversalLabel){
        for (int i = 0; i < Mv_Fifo.size();) {
            Mv_Fifo_Item mv_fifo_item;
            mv_fifo_item = Mv_Fifo[i];
            if (frameValidBuffer[mv_fifo_item._ref_idx]) {
                isWork = true;
                Mv_Fifo.erase(Mv_Fifo.begin() + i); //pop the exact mv_fifo_item
                //begin motion compesation
                int idx = mv_fifo_item._ref_idx;
                int x = mv_fifo_item._dst_x;
                int y = mv_fifo_item._dst_y;
                
                // add by liuxueyuan at 2020-1-17 15:30
                vector<pair<int, Data_Info> > Xs, Ys;
                if(x % 8){
                    int x0 = x/8 * 8, x1 = x0 + 8;
                    Xs.push_back(make_pair(x0, MC_L1_Cache.generate_cache_addr(idx, x0, 0)));
                    Xs.push_back(make_pair(x1, MC_L1_Cache.generate_cache_addr(idx, x1, 0)));
                }
                else{
                    Xs.push_back(make_pair(x, MC_L1_Cache.generate_cache_addr(idx, x, 0)));
                }
                if(y % 8){
                    int y0 = y/8 * 8, y1 = y0 + 8;
                    Ys.push_back(make_pair(y0, MC_L1_Cache.generate_cache_addr(idx, y0, 1)));
                    Ys.push_back(make_pair(y1, MC_L1_Cache.generate_cache_addr(idx, y1, 1)));
                }
                else{
                    Ys.push_back(make_pair(y, MC_L1_Cache.generate_cache_addr(idx, y, 1)));
                }
                L2_Request _l2req(mv_fifo_item, systimer);
                bool isHit = true;
                for(int m = 0; m < Xs.size(); ++m){
                    for(int n = 0; n < Ys.size(); ++n){
                        if(!MC_L1_Cache.check_cache_hit(Xs[m].second, Ys[n].second)){
                            // L1 cache miss
                            isHit = false;
                            Mv_Fifo_Item sublk(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x, mv_fifo_item._src_y, mv_fifo_item._ref_idx, Xs[m].first, Ys[n].first);
                            _l2req._mv_fifo_items.push_back(sublk);
                        }
                        else l1HitCnt++;
                    }
                }
                if(isHit){
                    // All hit in L1 cache
                    q_shifter.push(make_pair(mv_fifo_item, systimer+SHIFT_CYCLE));
                }
                else Pending_L2_request_list.push_back(_l2req);
                
                //end motion compesation
                MC_L1_Cache.update_frequency(); //update frequency
                MC_L2_Cache.update_frequency(); //use to update frequency
                break;
            }
            else i++;
        }
        mvFifoTraversalLabel = isWork;
    }
}

// simulate L2 cache
void Npusim::l2_cache_sim(){ //simulate the L2 cache hit and miss
    //check L2 request pending
    for (int i = 0; i < Pending_L2_request_list.size();) {
        if (Pending_L2_request_list[i]._return_cycle == systimer) {
            //begin check L2 cache hit
            bool isHit = true;
            Request tmp_req(Pending_L2_request_list[i]._ori_mv_item);
            for(vector<Mv_Fifo_Item>::iterator it=Pending_L2_request_list[i]._mv_fifo_items.begin();it!=Pending_L2_request_list[i]._mv_fifo_items.end();){
                int idx = it->_ref_idx;
                int x = it->_dst_x, y = it->_dst_y;
                Data_Info x_info, y_info;
                x_info = MC_L2_Cache.generate_cache_addr(idx, x, 0);
                y_info = MC_L2_Cache.generate_cache_addr(idx, y, 1);
                if(MC_L2_Cache.check_cache_hit(x_info, y_info)){
                    // L2 cache hit
                    l2HitCnt++;
                    MC_L1_Cache.replace_data(x_info, y_info);
                }
                else{
                    // L2 cache miss
                    isHit = false;
                    tmp_req._remain_num++;
                    int p_toDramReqList = maxid_dramReqList;

                    Dram_Info curDramReq(idx, x, y);
                    string s_curDraReq = bin2hex(generate_dram_addr(curDramReq));
                    bool isMergeAccReq = false;
                    for(int j=0; j<Dram_request_list.size(); j++){
                        if(s_curDraReq == Dram_request_list[j].first){
                            // the dram request already exists
                            mergeAccessCnt++;
                            isMergeAccReq = true;
                            p_toDramReqList = Dram_request_list[j].second;
                            break;
                        }
                    }
                    tmp_req._mv_fifo_items.push_back(make_pair(*it, p_toDramReqList));

                    if(!isMergeAccReq){
                        // new dram request
                        Dram_request_list.push_back(make_pair(s_curDraReq, p_toDramReqList));
                        maxid_dramReqList++;
                        // Send Read Request to Dram
                        // TODO: At the same cycle, dram can accept multiple requests, this is different from DramSim2 in design.
                        uint64_t dram_addr;
                        istringstream iss(s_curDraReq.substr(2));
                        iss >> hex >> dram_addr;
                        trans = new Transaction(DATA_READ, dram_addr, NULL);
                        if((*memorySystem).addTransaction(false, dram_addr)){
                            add_pending(trans, systimer);
                            trans = NULL;
                        }
                    }
                }
                it = Pending_L2_request_list[i]._mv_fifo_items.erase(it);
            }
            if(isHit){
                // All hit in (l1 cache and) l2 cache
                q_shifter.push(make_pair(Pending_L2_request_list[i]._ori_mv_item, systimer+SHIFT_CYCLE));
            }
            Pending_L2_request_list.erase(Pending_L2_request_list.begin()+i);
            if(tmp_req._remain_num > 0){
                Pending_request_list.push_back(tmp_req);
            }
        }
        else i++;
    }
    (*memorySystem).update();
}

void Npusim::controller(){
    int largeNNTimer = 0, smallNNTimer = 0;  // Name Used Before: temp_large_NN, temp_small_NN
    int decodeTimer = 0;                     // Name Used Before: temp_decode
    int curBframe = -1, i_curBframe = 0;
    int iplayer = 0, blayer = 0;
    uint64_t tmp_ipDecodedFramesAddr = ipDecodedFramesAddr;
    uint64_t tmp_bMcFramesAddr = bMcFramesAddr;
    while(!Decode_B_frame_idx.empty() || !ipNnList.empty()){
        systimer++;    // system clock move forward

        // process I/P frames
        if(!Decode_IP_frame_idx.empty()){
            // decode i/p frames
            decodeTimer++;
            if(decodeTimer >= decoder_sim()){
                int ipIdx = Decode_IP_frame_idx.front();
                ipNnList.push(ipIdx);
                Decode_IP_frame_idx.pop();
                decodeTimer = 0;
            }
        }
        if(!ipNnList.empty()){
            int ipIdx = ipNnList.front();
            if(!isLargeNetStart){
                largeNNTimer++;
                isLargeNetStart = true;
                // send write and read reqs at 0 layers
                int wTransNum = largeNet.calcTransPerLayer(iplayer, READ_WEIGHTS);
                uint64_t wAddr = lnnWeightStartAddr;
                for(int i = 0; i < wTransNum; i++){
                    wAddr += 64;
                    trans = new Transaction(DATA_READ, wAddr, NULL);
                    if((*memorySystem).addTransaction(false, wAddr)){
                        add_pending(trans, systimer);
                        pendingLnnReadPerLayer.push_back(wAddr);
                        trans = NULL;
                    }
                }
                int iTransNum = largeNet.calcTransPerLayer(iplayer, READ_INPUTS);
                for(int i = 1; i <= iTransNum; i++){
                    trans = new Transaction(DATA_READ, tmp_ipDecodedFramesAddr, NULL);
                    if((*memorySystem).addTransaction(false, tmp_ipDecodedFramesAddr)){
                        add_pending(trans, systimer);
                        pendingLnnReadPerLayer.push_back(tmp_ipDecodedFramesAddr);
                        trans = NULL;
                    }
                    tmp_ipDecodedFramesAddr += 64;
                }
                neuralnet_write_sim(largeNet, ipIdx, iplayer);
            }
            else{
                if(isLargeNetOk){
                    // this frame is complete
                    cout<<(frame_type_maptab[ipIdx]==P_FRAME?"P Frame: ":"I Frame: ")<<ipIdx<<" neural network completed. Current clock cycle: "<<systimer<<endl;
                    for(int i = 0; i < frameWidth + 10; i += 32) {
                        for(int j = 0; j < frameHeight; j += 8) {
                            Dram_Info ipDramInfo(ipIdx, i, j);
                            string s_ipDramReq = bin2hex(generate_dram_addr(ipDramInfo)).substr(2);
                            uint64_t ipDramAddr;
                            istringstream iss(s_ipDramReq);
                            iss >> hex >> ipDramAddr;
                            trans = new Transaction(DATA_WRITE, ipDramAddr, NULL);
                            if((*memorySystem).addTransaction(true, ipDramAddr)){
                                add_pending(trans, systimer);
                                pendingWriteReqList[ipIdx].push_back(ipDramAddr);
                                trans = NULL;
                            }
                        }
                    }
                    ipNnList.pop();
                    isLargeNetStart = false;
                    isLargeNetOk = false;
                    iplayer = 0;
                }
                else{
                    largeNNTimer++;
                    if(largeNNTimer >= largeNet.getExeCyclesPerLayer(iplayer) && isLnnLayerOk){
                        // This layer of neural network is done, the next layer can be start.
                        iplayer++;
                        neuralnet_read_sim(largeNet, ipIdx, iplayer);
                        neuralnet_write_sim(largeNet, ipIdx, iplayer);
                        largeNNTimer = 0;
                        isLnnLayerOk = false;
                    }
                }
            }
        }

        // process B frames
        if (!Decode_B_frame_idx.empty()) {
            if(curBframe == -1){
                for(vector<int>::iterator it=Decode_B_frame_idx.begin(); it!=Decode_B_frame_idx.end(); it++){
                    if(frameMcValid[*it]){
                        curBframe = *it;
                        i_curBframe = it - Decode_B_frame_idx.begin();
                        smallNNTimer++;
                        break;
                    }
                }
            }
            else{
                smallNNTimer++;
                if(!isSmallNetStart){
                    smallNNTimer++;
                    isSmallNetStart = true;
                    // send write and read requests at 0 layer
                    int wTransNum = smallNet.calcTransPerLayer(blayer, READ_WEIGHTS);
                    uint64_t wAddr = snnWeightStartAddr;
                    for(int i = 0; i < wTransNum; i++){
                        wAddr += 64;
                        trans = new Transaction(DATA_READ, wAddr, NULL);
                        if((*memorySystem).addTransaction(false, wAddr)){
                            add_pending(trans, systimer);
                            pendingLnnReadPerLayer.push_back(wAddr);
                            trans = NULL;
                        }
                    }
                    int iTransNum = smallNet.calcTransPerLayer(blayer, READ_INPUTS);
                    for(int i = 1; i <= iTransNum; i++){
                        trans = new Transaction(DATA_READ, tmp_bMcFramesAddr, NULL);
                        if((*memorySystem).addTransaction(false, tmp_bMcFramesAddr)){
                            add_pending(trans, systimer);
                            pendingLnnReadPerLayer.push_back(tmp_bMcFramesAddr);
                            trans = NULL;
                        }
                        tmp_bMcFramesAddr += 64;
                    }
                    neuralnet_write_sim(smallNet, curBframe, blayer);
                }
                else{
                    if(isSmallNetOk){
                        cout<<"B Frame: "<<curBframe<<" neural network completed. Current clock cycle: "<<systimer<<endl;
                        for(int i = 0; i < frameWidth+10; i += 32){
                            for(int j = 0; j < frameHeight; j += 8){
                                Dram_Info bDramInfo(curBframe, i, j);
                                string s_bDramReq = bin2hex(generate_dram_addr(bDramInfo)).substr(2);
                                uint64_t bDramAddr;
                                istringstream iss(s_bDramReq);
                                iss >> hex >> bDramAddr;
                                trans = new Transaction(DATA_WRITE, bDramAddr, NULL);
                                if((*memorySystem).addTransaction(true, bDramAddr)){
                                    add_pending(trans, systimer);
                                    pendingWriteReqList[curBframe].push_back(bDramAddr);
                                    trans = NULL;
                                }
                            }
                        }
                        Decode_B_frame_idx.erase(Decode_B_frame_idx.begin() + i_curBframe);
                        curBframe = -1;
                        isSmallNetStart = false;
                        isSmallNetOk = false;
                        blayer = 0;
                    }
                    else{
                        smallNNTimer++;
                        if(smallNNTimer >= smallNet.getExeCyclesPerLayer(blayer) && isSnnLayerOk){
                            blayer++;
                            neuralnet_read_sim(smallNet, curBframe, blayer);
                            neuralnet_write_sim(smallNet, curBframe, blayer);
                            smallNNTimer = 0;
                            isSnnLayerOk = false;
                        }
                    }
                }
            }
            
            // execute MC process, update frameMcValid
            if(!q_postMappedBuffer.empty()){
                if(systimer >= q_postMappedBuffer.front().second){
                    int bidx = q_postMappedBuffer.front().first._b_idx;
                    blkInBframe[bidx]--;
                    q_postMappedBuffer.pop();
                    if(blkInBframe[bidx] == 0){
                        frameMcValid[bidx] = 1;
                    }
                }
            }

            if(!q_shifter.empty()){
                if(systimer >= q_shifter.front().second){
                    Mv_Fifo_Item mp_item = q_shifter.front().first;
                    if(mp_item.isBidPredFrame){
                        // 双向预测帧
                        bool isExist = false;
                        for(vector<Mv_Fifo_Item>::iterator it = mappedBuffer.begin(); it != mappedBuffer.end();){
                            if(it->_b_idx == mp_item._b_idx && it->_src_x == mp_item._src_x && it->_src_y == mp_item._src_y){
                                isExist = true;
                                it = mappedBuffer.erase(it);
                                q_postMappedBuffer.push(make_pair(mp_item, systimer+POST_MPBCALC_CYCLE));
                                break;
                            }
                            else it++;
                        }
                        if(!isExist){
                            mappedBuffer.push_back(q_shifter.front().first);
                        }
                    }
                    else{
                        q_postMappedBuffer.push(make_pair(mp_item, systimer+POST_MPBCALC_CYCLE));
                    }
                    q_shifter.pop();
                }
            }
            // if(systimer%5==0){
            //     l2_cache_sim();
            //     l1_cache_sim();
            // }
            l2_cache_sim();
            
            l1_cache_sim();
            
        }
    }
    cout<<"Controller finished. Simulation time-consuming: "<<systimer<<endl;
    cout<<"L1 cache hits: "<<l1HitCnt<<", L2 cache hits: "<<l2HitCnt<<", Merge Access: "<<mergeAccessCnt<<endl;
}

void Npusim::test(){
    uint64_t dram_addr = 0;
    trans = new Transaction(DATA_READ, dram_addr, NULL);
    if((*memorySystem).addTransaction(false, dram_addr)){
        add_pending(trans, systimer);
        trans = NULL;
    }
    delete trans;
    dram_addr = 100;
    trans = new Transaction(DATA_READ, dram_addr, NULL);
    if((*memorySystem).addTransaction(false, dram_addr)){
        add_pending(trans, systimer);
        trans = NULL;
    }
    delete trans;
    dram_addr = 35679;
    trans = new Transaction(DATA_READ, dram_addr, NULL);
    if((*memorySystem).addTransaction(false, dram_addr)){
        add_pending(trans, systimer);
        trans = NULL;
    }
    delete trans;
    dram_addr = 38982321;
    trans = new Transaction(DATA_WRITE, dram_addr, NULL);
    if((*memorySystem).addTransaction(true, dram_addr)){
        add_pending(trans, systimer);
        trans = NULL;
    }
    while (1)
    {
        (*memorySystem).update();
    }
}

void Npusim::test_v2(){
    for(int i = 0; i < 100; i++){
        uint64_t dram_addr = 0;
        trans = new Transaction(DATA_READ, dram_addr, NULL);
        if((*memorySystem).addTransaction(false, dram_addr)){
            add_pending(trans, systimer);
            trans = NULL;
        }
        dram_addr+=128;
        delete trans;
        systimer++;
        (*memorySystem).update();
    }
    while (1)
    {
        (*memorySystem).update();
    }
    
    
}


int main(int argc, char* argv[]){
    Npusim npu;
    string bpath = "b/", ppath = "p/", mvpath = "./mvs/";
    bpath.append(argv[1]), ppath.append(argv[1]), mvpath.append(argv[1]);
    mvpath += ".csv";
    npu.init("./idx", bpath, ppath, mvpath);
    npu.controller();
    // npu.test_v2();
    return 0;
}