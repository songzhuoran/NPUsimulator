#pragma once

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <string>
#include <stdint.h>
#include <map>
#include <cmath>
#include "PrintMacros.h"
#include "request.h"
using namespace std;

class NeuralNetLayer{
public:
    int id;
    int dependency;
    int weights;
    int inputs;
    int outputs;
    long long computations;

    NeuralNetLayer(int _id, int _dep, int _w, int _i, int _o, long long _c):id(_id), dependency(_dep), weights(_w), inputs(_i), outputs(_o), computations(_c){};
};

// Type of neural network, SNN_T-small NN, LNN_T-large NN
enum NnType{
    SNN_T, LNN_T
};

enum NpuIOType{
    READ_INPUTS, READ_WEIGHTS, WRITE_OUTPUTS
};

class NeuralNetwork{
private:
    int layersNum;       // number of layers
    NnType type;         // type of neural network
    int macsNum;         // number of MACs
    int wFifoSize; // capacity of weight fifo(on-chip memory)
    int unifiedBufferSize;

public:
    vector<NeuralNetLayer> netArch;
    vector<uint64_t> outputAddrOffset;
    vector<uint64_t> weightAddrOffset;

    NeuralNetwork(){};
    void init(NnType t);
    int getLayersNum();
    NnType getNetType();
    int calcTransPerLayer(int curLayer, NpuIOType io_t);
    long long getExeCyclesPerLayer(int curLayer);
};

// class NetworkLabel{
// public:
//     int frameIdx;
//     bool isframeOk;
//     int layersNum;
//     vector<bool> isLayerOk;
//     bool isInitial;
//     NetworkLabel():isframeOk(false), isInitial(false){}
// };