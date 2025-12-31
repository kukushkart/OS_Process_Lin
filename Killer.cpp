#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <cstring>

using namespace std;

vector<string> split(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

string GetProcessNameByPid(int pid) {
    string path = "/proc/" + to_string(pid) + "/comm";
    ifstream file(path);
    string name = "";
    if (file.is_open()) {
        getline(file, name);
        if (!name.empty() && name.back() == '\n') {
            name.pop_back();
        }
    }
    return name;
}

bool KillProcessById(int processId) {
    if (kill(processId, SIGKILL) == 0) {
        return true;
    }
    return false;
}

int KillProcessByName(const string& processName) {
    int killedCount = 0;
    DIR* dir = opendir("/proc");
    if (dir == nullptr) {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            string currentName = GetProcessNameByPid(pid);

            string currentNameLower = currentName;
            transform(currentNameLower.begin(), currentNameLower.end(), currentNameLower.begin(), ::tolower);

            string targetNameLower = processName;
            transform(targetNameLower.begin(), targetNameLower.end(), targetNameLower.begin(), ::tolower);

            if (currentNameLower == targetNameLower) {
                if (KillProcessById(pid)) {
                    killedCount++;
                }
            }
        }
    }

    closedir(dir);
    return killedCount;
}

int main(int argc, char* argv[]) {
    const char* envBuffer = getenv("PROC_TO_KILL");

    if (envBuffer != nullptr) {
        string envValue(envBuffer);
        vector<string> processNames = split(envValue, ',');

        for (const string& procName : processNames) {
            string trimmedName = procName;
            if (trimmedName.find_first_not_of(" \t") != string::npos) {
                trimmedName.erase(0, trimmedName.find_first_not_of(" \t"));
                trimmedName.erase(trimmedName.find_last_not_of(" \t") + 1);
            }

            if (!trimmedName.empty()) {
                KillProcessByName(trimmedName);
            }
        }
    }

    for (int i = 1; i < argc; i++) {
        string arg(argv[i]);

        if (arg == "--id" && i + 1 < argc) {
            int processId = atoi(argv[i + 1]);
            KillProcessById(processId);
            i++;
        }
        else if (arg == "--name" && i + 1 < argc) {
            string processName(argv[i + 1]);
            KillProcessByName(processName);
            i++;
        }
    }

    return 0;
}