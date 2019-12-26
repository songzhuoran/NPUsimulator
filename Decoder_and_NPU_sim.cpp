#include "sim.h"
#include <string>
#include <algorithm>
#include "math.h"

using namespace std;

int total_cycle = 0;
int num_frame = 4;
vector<int> Valid_NN_Frame(num_frame,0); //neural network process is completed
vector<int> Valid_MC_Frame(num_frame,0); //motion compensation process is completed
vector<int> Valid_Decode_Frame(num_frame,1); //decode process is completed

vector<int> Decode_frame_idx; // the whole decode order
vector<int> Decode_IP_frame_idx; // IP order
vector<int> Decode_B_frame_idx; // B order
vector<int> Frame_type; //define the frame type of each frame: 0: I frame; 1: P frame; 2: B frame
vector<Request> Pending_request_list; //pending request of DRAM
vector<Dram_Request> Dram_Request_list;
vector<Mv_Fifo_Item> Mv_Fifo; // need to init!!(including mv_fifo_item)
vector<L2_Request> Pending_L2_request_list; //pending request of L2 cache


string bin2hex(const string& bin){
    string res = "0x";
    for (int i = 0; i < bin.size(); i += 4) {
        string tmp = bin.substr(i, 4);
        if (tmp[0] == '0') {
            if (tmp == "0000")res += '0';
            else if (tmp == "0001") res += '1';
            else if (tmp == "0010") res += '2';
            else if (tmp == "0011") res += '3';
            else if (tmp == "0100") res += '4';
            else if (tmp == "0101") res += '5';
            else if (tmp == "0110") res += '6';
            else res += '7';
        }
        else {
            if (tmp == "1000") res += '8';
            else if (tmp == "1001") res += '9';
            else if (tmp == "1010") res += 'A';
            else if (tmp == "1011") res += 'B';
            else if (tmp == "1100") res += 'C';
            else if (tmp == "1101") res += 'D';
            else if (tmp == "1110") res += 'E';
            else res += 'F';
        }
    }
    return res;
}


int NN_sim(int frame_type){ //simulate the neural network
    int NN_cycle = 0;
    int array_size = 128*128;
    if(frame_type==0){
        //NN_cycle = 3805503; //‭62349373440/(128*128)
        NN_cycle = 100; //test case
    }
    else if(frame_type==1){
        //NN_cycle = 3805503; //need to add
        NN_cycle = 100; //test case
    }
    else{
        //NN_cycle = 380550; //need to add
        NN_cycle = 10; //test case
    }
    return NN_cycle;
}


Data_Info Generate_data_info(int x){ //generate cache data, including tag, index and bank id
    string str_x0 = dec2bin(x, 12);
    string tag_x0 = str_x0.substr(0,6);
    string index_x0 = str_x0.substr(6,2);
    string bank_x0 = str_x0.substr(8,1);
    int i_tag_x0 = atoi(tag_x0.c_str());
    int i_index_x0 = atoi(index_x0.c_str());
    int i_bank_x0 = atoi(bank_x0.c_str());
    Data_Info data_info;
    data_info.init_data_info(i_tag_x0,i_index_x0,i_bank_x0);
    return data_info;
}

void DRAM_sim(MC_Cache& mc_cache1, MC_Cache& mc_cache2){
    // cout<<"total_cycle = "<<total_cycle<<endl;
    // cout<<"Pending_request_list.size" << Pending_request_list.size()<<endl;
    for (int i = 0; i < Dram_Request_list.size();) {
        // cout<<"pending list is not empty, total_cycle = "<<total_cycle<<endl;
        if (Dram_Request_list[i]._return_cycle == total_cycle) {
            cout<<"return cycle: "<<total_cycle<<", addr: "<<bin2hex(Dram_Request_list[i]._Dram_addr)<<endl;//test case
            for (int j = 0; j < Pending_request_list.size();) {
                if (Dram_Request_list[i]._Dram_addr == Pending_request_list[j]._Dram_addr) {
                    // cout << "Step in the 442" << endl;
                    int tmp_idx = Pending_request_list[j]._mv_fifo_item._ref_idx;
                    int tmp_x = Pending_request_list[j]._mv_fifo_item._dst_x;
                    int tmp_y = Pending_request_list[j]._mv_fifo_item._dst_y;
                    Data_Info tmp_x0_info;
                    Data_Info tmp_y0_info;
                    tmp_x0_info = Generate_data_info(tmp_x);
                    tmp_y0_info = Generate_data_info(tmp_y);
                    mc_cache1.replace_data(tmp_x0_info,tmp_y0_info);
                    // mc_cache1.replace_data(tmp_x0_info._i_b, tmp_y0_info._i_b, tmp_x0_info._i_index, tmp_y0_info._i_index, tmp_x0_info._i_tag, tmp_y0_info._i_tag);// replace L1 cache data
                    mc_cache2.replace_data(tmp_x0_info,tmp_y0_info);
                    // mc_cache2.replace_data(tmp_x0_info._i_b, tmp_y0_info._i_b, tmp_x0_info._i_index, tmp_y0_info._i_index, tmp_x0_info._i_tag, tmp_y0_info._i_tag);// replace L2 cache data
                    Pending_request_list.erase(Pending_request_list.begin() + j);
                }
                else{
                    j++;
                }
            }
            Dram_Request_list.erase(Dram_Request_list.begin() + i);
        }
        else i++;
    }
}

int Decode_sim(){
    int frequency = 3; //300MHz
    int FPS = 6; //60fps
    int cycle = 3/6*pow(10,7);
    return cycle;
}


void MC_L2_sim(MC_Cache& mc_cache1, MC_Cache& mc_cache2){ //simulate the L2 cache hit and miss
    //check L2 request pending
    for (int i = 0; i < Pending_L2_request_list.size();) {
        if (Pending_L2_request_list[i]._return_cycle == total_cycle) {
            //begin check L2 cache hit
            Mv_Fifo_Item temp_mv_fifo_item = Pending_L2_request_list[i]._mv_fifo_item;
            int idx = temp_mv_fifo_item._ref_idx;
            int x = temp_mv_fifo_item._dst_x;
            int y = temp_mv_fifo_item._dst_y;
            Data_Info x0_info;
            Data_Info y0_info;
            x0_info = Generate_data_info(x);
            y0_info = Generate_data_info(y);
            Dram_Info dram_info(idx,x,y);// generate DRAM information
            if(mc_cache2.check_cache_hit(x0_info,y0_info)){ //L2 cache hit
                cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y<<"), L2 hit"<<endl; //test case
                mc_cache1.replace_data(x0_info,y0_info);
                // mc_cache1.replace_data(x0_info._i_b, y0_info._i_b, x0_info._i_index, y0_info._i_index, x0_info._i_tag, y0_info._i_tag);// replace L1 cache data
            }
            else{ //L2 cache miss
                //pending L2 request list
                cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y<<"), L2 miss"<<endl;  //test case

                Request temp_request(temp_mv_fifo_item,dram_info);
                Pending_request_list.push_back(temp_request); // pending the corresponding request of the exact mv item
                //pending DRAM request list
                int return_cycle = 11 + total_cycle;
                int flag = 0;
                for(int i =0; i < Dram_Request_list.size(); i++){
                    if(Dram_Request_list[i]._Dram_addr.compare(temp_request._Dram_addr)==0){ //equal to Dram_addr
                        flag = 1;
                    }
                }
                if(flag == 0){
                    Dram_Request dram_request;
                    dram_request.init_DRAM_Request(temp_request._Dram_addr, return_cycle);
                    cout<<"request cycle: "<<total_cycle<<", addr: "<<bin2hex(temp_request._Dram_addr)<<endl;//test case

                    Dram_Request_list.push_back(dram_request);
                }
            }
            //end check L2 cache hit
            Pending_L2_request_list.erase(Pending_L2_request_list.begin() + i);
        }
        else i++;
    }
}

void MC_L1_sim(MC_Cache& mc_cache1, MC_Cache& mc_cache2){ //simulate the L1 cache hit and miss
    for (int i = 0; i < Mv_Fifo.size();) {
        Mv_Fifo_Item mv_fifo_item;
        mv_fifo_item = Mv_Fifo[i];
        if (Valid_NN_Frame[mv_fifo_item._ref_idx]) {
            mc_cache1.init_flat_hit(); //upate flag hit, use to update frequency
            mc_cache2.init_flat_hit(); //upate flag hit, use to update frequency
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
                    x0_info = Generate_data_info(x0);
                    x1_info = Generate_data_info(x1);
                    y0_info = Generate_data_info(y0);
                    y1_info = Generate_data_info(y1); // generate aligned information
                    Dram_Info dram_info1(idx,x0,y0);
                    Dram_Info dram_info2(idx,x0,y1);
                    Dram_Info dram_info3(idx,x1,y0);
                    Dram_Info dram_info4(idx,x1,y1);// generate DRAM information
                    bool hit_flag = true;
                    if(!mc_cache1.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        hit_flag = false;  //test
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x0<<", "<<y0<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x0, y0);
                        //pending L2 request
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x0<<", "<<y0<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;

                    if(!mc_cache1.check_cache_hit(x0_info,y1_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x0<<", "<<y1<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x0, y1);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x0<<", "<<y1<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;

                    if(!mc_cache1.check_cache_hit(x1_info,y0_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x1<<", "<<y0<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x1, y0);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x1<<", "<<y0<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;

                    if(!mc_cache1.check_cache_hit(x1_info,y1_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x1<<", "<<y1<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x1, y1);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x1<<", "<<y1<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;
                }
                else{
                    // horizontal: across 2 block
                    int x0 = x/8 * 8;
                    int x1 = x0 + 8;
                    Data_Info x0_info;
                    Data_Info x1_info;
                    Data_Info y0_info;
                    x0_info = Generate_data_info(x0);
                    x1_info = Generate_data_info(x1);
                    y0_info = Generate_data_info(y);
                    Dram_Info dram_info1(idx,x0,y);
                    Dram_Info dram_info2(idx,x1,y);// generate DRAM information
                    bool hit_flag = true;   //test case
                    if(!mc_cache1.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x0<<", "<<y<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x0, y);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x0<<", "<<y<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;

                    if(!mc_cache1.check_cache_hit(x1_info,y0_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x1<<", "<<y<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x1, y);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x1<<", "<<y<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;
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
                    x0_info = Generate_data_info(x);
                    y0_info = Generate_data_info(y0);
                    y1_info = Generate_data_info(y1);
                    Dram_Info dram_info1(idx,x,y0);
                    Dram_Info dram_info2(idx,x,y1);
                    bool hit_flag = true;
                    if(!mc_cache1.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y0<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x, y0);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y0<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;

                    if(!mc_cache1.check_cache_hit(x0_info,y1_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y1<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x, y1);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y1<<"), L1 hit"<<endl; //test case
                    }
                    hit_flag = true;
                }
                else{
                    // one block
                    Data_Info x0_info;
                    Data_Info y0_info;
                    x0_info = Generate_data_info(x);
                    y0_info = Generate_data_info(y);
                    Dram_Info dram_info1(idx,x,y);
                    bool hit_flag = true;
                    if(!mc_cache1.check_cache_hit(x0_info,y0_info)){
                        // L1 miss
                        hit_flag = false;
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y<<"), L1 miss"<<endl; //test case
                        Mv_Fifo_Item temp_mv_fifo_item;
                        temp_mv_fifo_item.init_mv_fifo_item(mv_fifo_item._b_idx, mv_fifo_item._width, mv_fifo_item._height, mv_fifo_item._src_x,mv_fifo_item._src_y, idx, x, y);
                        L2_Request temp_request(temp_mv_fifo_item,total_cycle);
                        Pending_L2_request_list.push_back(temp_request);
                    }
                    //test case 
                    if(hit_flag == true){
                        cout<<"cycle: "<<total_cycle<<", ("<<idx<<", "<<x<<", "<<y<<"), L1 hit"<<endl; //test case
                    }
                }
            }
            //end motion compesation
            mc_cache1.update_frequency(); //update frequency
            mc_cache2.update_frequency(); //use to update frequency
            break;
        }
        else {
            i++;
            continue;
        }
    }
}

void simulate_cycle(){
    MC_Cache MC_L1_Cache(2, 2, 1, 1, 4, 4, 8, 8);
    MC_Cache MC_L2_Cache(2, 2, 1, 1, 8, 8, 8, 8);
    int temp_large_NN = 0;
    int temp_decode = 0;
    int temp_small_NN = 0;
    while(!Decode_IP_frame_idx.empty()||!Decode_B_frame_idx.empty()){
        total_cycle++;
        // cout<<"Decode_B_frame_idx.size = "<<Decode_B_frame_idx.size()<<", ";
        // cout<<"Decode_IP_frame_idx.size = "<<Decode_IP_frame_idx.size()<<endl;
        if (!Decode_IP_frame_idx.empty()) {
            int frame_IP_idx = Decode_IP_frame_idx.front(); //current frame
            int frame_IP_type = Frame_type[frame_IP_idx];
            if (Valid_Decode_Frame[frame_IP_idx]) {
                temp_large_NN++;
                if (temp_large_NN > NN_sim(frame_IP_type)) {
                    Valid_NN_Frame[frame_IP_idx] = 1;
                    Decode_IP_frame_idx.erase(Decode_IP_frame_idx.begin()); // pop the exact I or P frame idx
                    temp_large_NN = 0;
                }
            }
            // else{ // I and P frame need to be decoded, while B frame does not need
            //    temp_decode++;
            //    if(temp_decode>Decode_sim()){
            //        Valid_Decode_Frame[frame_IP_idx] = 1;
            //        temp_decode = 0;
            //    }
            // }
        }
        if (!Decode_B_frame_idx.empty()) {
            int frame_B_idx = Decode_B_frame_idx.front(); //current frame
            int frame_B_type = Frame_type[frame_B_idx];
            if (Valid_MC_Frame[frame_B_idx]) { //execute NN
                // cout << "step in B" << endl;//test case
                temp_small_NN++;
                if (temp_small_NN > NN_sim(frame_B_type)) {
                    Valid_NN_Frame[frame_B_idx] = 1;
                    Decode_B_frame_idx.erase(Decode_B_frame_idx.begin()); // pop the exact B frame idx
                    temp_small_NN = 0;
                }
            }
            // execute MC process, update Valid_MC_Frame
            DRAM_sim(MC_L1_Cache, MC_L2_Cache);
            MC_L2_sim(MC_L1_Cache, MC_L2_Cache);
            MC_L1_sim(MC_L1_Cache, MC_L2_Cache);

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
            //cout << "Pending list size = " << Pending_request_list.size() << endl;
            for (int j = 0; j < Pending_request_list.size(); j++) {
                if (frame_B_idx != Pending_request_list[j]._mv_fifo_item._b_idx) {
                    continue;
                }
                else {
                    temp_j = j;
                    break;
                }
            }
            // cout<<"tmp_i = "<<temp_i<<", tmp_j = "<<temp_j<<endl;
            if (temp_i == -1 && temp_j == -1) {
                Valid_MC_Frame[frame_B_idx] = 1;
                // cout << "Valid_MC_Frame[" << frame_B_idx << "] = " << Valid_MC_Frame[frame_B_idx] << endl;
            }
            
        }
    }
}

// init test case
void sysini(){
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
    Frame_type.push_back(0);
    Frame_type.push_back(2), Frame_type.push_back(2);
    Frame_type.push_back(1);

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
    sysini();
    simulate_cycle();
    // vector<int> test;
    // test.push_back(17);
    // test.push_back(67);
    // for(int i=0;i<2;i++){
    //     test.erase(test.begin());
    // }
    // cout<<test.size()<<endl;
    return 0;
}