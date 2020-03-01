#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<cmath>
#include<sstream>
#include<map>
#include<io.h>
using namespace std;

typedef struct analysisItem {
    double sp;  //speedup
}item;

map<string, int> baselineCycles;

void getFiles(string path, vector<string>& files)
{
    long hFile = 0;
    struct _finddata_t fileinfo;
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do {
            if ((fileinfo.attrib & _A_SUBDIR))
            {
                if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
                    getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
            }
            else {
                files.push_back(p.assign(path).append("\\").append(fileinfo.name));
                //files.push_back(fileinfo.name);
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

void loadBaselineCycles(string baselinePath) {
    ifstream inFile(baselinePath, ios::in);
    string lineStr;
    while (getline(inFile, lineStr))
    {
        stringstream ss(lineStr);
        string str;
        vector<string> lineArray;
        while (getline(ss, str, ','))
            lineArray.push_back(str);
        baselineCycles[lineArray[0]] = atoi(lineArray[1].c_str());
    }
}

// calc speedup
void logAnalysis(item& res, string logPath) {
    FILE* fp;
    char sline[1024];
    size_t rd;
    int i;
    fopen_s(&fp, logPath.c_str(), "r");
    if (fp == NULL) {
        exit(1);
    }
    fseek(fp, 0, SEEK_END);
    if (ftell(fp) > 1023)
        fseek(fp, -1023, SEEK_CUR);
    else
        fseek(fp, -ftell(fp), SEEK_CUR);
    memset(sline, 0, sizeof(sline));
    rd = fread(sline, 1, 1023, fp);
    if (sline[rd - 1] == '\n')
        sline[rd - 1] = 0;
    for (i = (1023 > rd ? rd : 1023); i >= 0; i--) {
        if (sline[i] == '\n') {
            break;
        }
    }
    if (i < 0) {
        printf("this line is too long....\n");
        exit(1);
    }
    string lastLine = sline + i + 1;
    string logName = logPath.substr(logPath.find_last_of('\\') + 1, logPath.find_last_of('.') - logPath.find_last_of('\\') - 1);
    int timeConsume = atoi(lastLine.substr(lastLine.find_first_of(':') + 2).c_str());
    int baselineTime = baselineCycles[logName];
    res.sp = 1.0*baselineTime / timeConsume;
}


int main() {
    string baselinePath = "C:\\Users\\lxy10\\Desktop\\NPUsimulator\\log\\baselineCycles.csv";
    loadBaselineCycles(baselinePath);
    vector<string> bfiles;
    string defaultRoot = "C:\\Users\\lxy10\\Desktop\\log";
    getFiles(defaultRoot, bfiles);
    item res[50];
    ofstream oFile;
    oFile.open("npusimPerformanceAnalysis.csv", ios::out);
    for (size_t i = 0; i < bfiles.size(); i++) {
        logAnalysis(res[i], bfiles[i]);
        string name = bfiles[i];
        oFile << name.substr(name.find_last_of('\\') + 1, name.find_last_of('.') - name.find_last_of('\\') - 1) << "," << res[i].sp << endl;
    }
    oFile.close();
    return 0;
}