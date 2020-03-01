#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<cmath>
#include<map>
#include<io.h>
using namespace std;

const int decodeCycles = 0.5 * pow(10, 7);
const int largeNnCycles = 3805503;

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
                //files.push_back(p.assign(path).append("\\").append(fileinfo.name));
                files.push_back(fileinfo.name);
            }
        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
}

void getNumFrames(map<string, int>& res, string bpath, string ppath, string ipath = "") {
    int numFrames = 0;
    ifstream bFile(bpath.c_str());
    if (!bFile.is_open()) {
        cout << "Can not open b-file: " << bpath << endl;
        exit(1);
    }
    string bline;
    while (getline(bFile, bline)) {
        numFrames++; // b frame
    }
    bFile.close();
    ifstream pFile(ppath.c_str());
    if (!pFile.is_open()) {
        cout << "Can not open p-file: " << ppath << endl;
        exit(1);
    }
    string pline;
    while (getline(pFile, pline)) {
        numFrames++; // p frame (may include 1 iframe)
    }
    pFile.close();
    if (!ipath.empty()) {
        string iline;
        ifstream iFile(ipath.c_str());
        if (!iFile.is_open()) {
            cout << "Can not open i-file: " << ipath << endl;
            exit(1);
        }
        while (getline(iFile, iline)) {
            numFrames++; // i frame
        }
        iFile.close();
    }
    res[bpath.substr(bpath.find_last_of("\\") + 1)] = numFrames;
}

int main() {
    const int CONSTNUM = decodeCycles + largeNnCycles;
    vector<string> bfiles, pfiles;
    string defaultRoot = "..\\idx";
    string bFilePath = defaultRoot + "\\b\\";
    string pFilePath = defaultRoot + "\\p\\";
    getFiles(bFilePath, bfiles);
    map<string, int> res;
    for (size_t i = 0; i < bfiles.size(); i++) {
        getNumFrames(res, bFilePath + bfiles[i], pFilePath + bfiles[i]);
    }
    ofstream oFile;
    oFile.open("baselineCycles.csv", ios::ate);
    for (map<string, int>::iterator it = res.begin(); it != res.end(); ++it) {
        oFile << it->first << "," << it->second * CONSTNUM << endl;
    }
    oFile.close();
    return 0;
}