#include "neuralNetworkSim.h"

void NeuralNetwork::init(NnType t){
    // Load npu.ini
    ifstream iniFile;
    iniFile.open("./ini/npu.ini");
    if(!iniFile.is_open()){
        cout<<"Cannot open npu.ini file!"<<endl;
    }
    string line;
    map<string, long long> npuConfig;
    while(getline(iniFile, line)){
        if(line != "\r"){
            int equpos = line.find('=');

            string key = line.substr(0, equpos);
            key.erase(0, key.find_first_not_of(" "));
            key.erase(key.find_last_not_of(" ") + 1);

            string val = line.substr(equpos + 1);
            val.erase(0, val.find_first_not_of(" "));
            val.erase(val.find_last_not_of(" ") + 1);

            istringstream iss(val);
            long long llval;
            iss >> llval;
            npuConfig[key] = llval;
        }
    }
    iniFile.close();
    if(t == SNN_T){
        type = SNN_T;
        layersNum = npuConfig["SMALLNET_LAYER_NUM"];
        macsNum = npuConfig["SMALLNET_MAC_NUM"];
        wFifoSize = npuConfig["SMALLNET_WEIGHT_FIFO"];
        unifiedBufferSize = npuConfig["SMALLNET_UNIFIED_BUFFER"];

        ifstream archFile("./ini/smallNnConfig.csv", ios::in);  
        string lineStr;
        while(getline(archFile, lineStr))  
        {  
            stringstream ss(lineStr);  
            string str;  
            vector<string> lineArray;  
            while (getline(ss, str, ','))  
                lineArray.push_back(str);
            long long com;
            istringstream iss(lineArray[12]);
            iss >> com;
            NeuralNetLayer netLayer(atoi(lineArray[0].c_str()), atoi(lineArray[13].c_str()), atoi(lineArray[9].c_str()), atoi(lineArray[10].c_str()), atoi(lineArray[11].c_str()), com);
            netArch.push_back(netLayer);
        }
        archFile.close();
    }
    else{
        type = LNN_T;
        layersNum = npuConfig["LARGENET_LAYER_NUM"];
        macsNum = npuConfig["LARGENET_MAC_NUM"];
        wFifoSize = npuConfig["LARGENET_WEIGHT_FIFO"];
        unifiedBufferSize = npuConfig["LARGENET_UNIFIED_BUFFER"];
        cout<<"layersNum = "<<layersNum<<endl;

        ifstream archFile("./ini/largeNnConfig.csv", ios::in);  
        string lineStr;
        while(getline(archFile, lineStr))  
        {  
            stringstream ss(lineStr);  
            string str;  
            vector<string> lineArray;  
            while (getline(ss, str, ','))  
                lineArray.push_back(str);
            long long com;
            istringstream iss(lineArray[12]);
            iss >> com;
            NeuralNetLayer netLayer(atoi(lineArray[0].c_str()), atoi(lineArray[13].c_str()), atoi(lineArray[9].c_str()), atoi(lineArray[10].c_str()), atoi(lineArray[11].c_str()), com);
            netArch.push_back(netLayer);
        }
        archFile.close();
    }
    uint64_t offset = 0;
    for(int i = 0; i < netArch.size(); i++){
        outputAddrOffset.push_back(offset);
        offset += netArch[i].outputs;
    }
    offset = 0;
    for(int i = 0; i < netArch.size(); i++){
        weightAddrOffset.push_back(offset);
        offset += netArch[i].weights;
    }
}

int NeuralNetwork::getLayersNum(){
    return layersNum;
}

NnType NeuralNetwork::getNetType(){
    return type;
}

int NeuralNetwork::calcTransPerLayer(int curLayer, NpuIOType io_t){
    NeuralNetLayer thisLayer = netArch[curLayer];
    int s_weight = thisLayer.weights;  // size of weights
    int s_input  = thisLayer.inputs;   // size of inputs
    int s_output = thisLayer.outputs;  // size of outputs
    // cout<<"current layer = "<<curLayer<<", s_weight = "<<s_weight<<", s_input"<<s_input<<", s_output= "<<s_output<<endl;
    int transNum;
    if(io_t == READ_WEIGHTS){      
        // 64 Bytes/Transaction, load weights per layer from dram
        transNum = s_weight >> 6;
    }
    else if(io_t == READ_INPUTS){
        if(curLayer==0 || netArch[curLayer-1].id!=thisLayer.dependency){
            // if current layer is Layer-0 or the previous layer does not depend on this layer, then read all inputs from dram.
            transNum = s_input >> 6;
        }
        else{
            // the previous layer depends on this layer, read inputs from half of unified buffer and dram.
            if(s_input <= (unifiedBufferSize >> 1)){
                // inputs is tiny enough to be held in the on-chip buffer
                transNum = 0;
            }
            else{
                transNum = (s_input - (unifiedBufferSize >> 1)) >> 6;
            }
        }
    }
    else{
        // io_t == WRITE_OUTPUTS
        if(curLayer==layersNum-1 || netArch[curLayer+1].dependency!=thisLayer.id){
            // if current layer is the last layer or the next layer does not depend on current layer, then all outputs should be write back to dram.
            transNum = s_output >> 6;
        }
        else{
            // if the next layer depends on current layer, then outputs can be write to both on-chip buffer and dram.
            if(s_output <= (unifiedBufferSize >> 1)){
                // outpus is small enough that can be held in on-chip buffer
                transNum = 0;
            }
            else{
                transNum = (s_output - (unifiedBufferSize >> 1)) >> 6;
            }
        }
    }
    return transNum;
}

uint64_t NeuralNetwork::getExeCyclesPerLayer(int curLayer){
    NeuralNetLayer thisLayer = netArch[curLayer];
    uint64_t computeNums = thisLayer.computations;
    return computeNums / macsNum;
}