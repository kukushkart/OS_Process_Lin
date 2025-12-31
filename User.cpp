#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <algorithm>

using namespace std;

string GetProcessNameByPidUser(int pid) {
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

bool IsProcessRunning(const string& processName) {
    DIR* dir = opendir("/proc");
    if (dir == nullptr) return false;

    struct dirent* entry;
    bool found = false;

    while ((entry = readdir(dir)) != nullptr) {
        if (isdigit(entry->d_name[0])) {
            int pid = atoi(entry->d_name);
            string name = GetProcessNameByPidUser(pid);

            if (strcasecmp(name.c_str(), processName.c_str()) == 0) {
                found = true;
                break;
            }
        }
    }
    closedir(dir);
    return found;
}

bool IsProcessRunningById(int pid) {
    return (kill(pid, 0) == 0);
}

pid_t StartProcess(string cmdLine, bool waitForExit = false) {
    vector<string> args;
    string token;
    size_t pos = 0;
    while ((pos = cmdLine.find(' ')) != string::npos) {
        token = cmdLine.substr(0, pos);
        if (!token.empty()) args.push_back(token);
        cmdLine.erase(0, pos + 1);
    }
    if (!cmdLine.empty()) args.push_back(cmdLine);

    vector<char*> c_args;
    for (const auto& arg : args) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    pid_t pid = fork();

    if (pid == -1) {
        return 0;
    }
    else if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        execvp(c_args[0], c_args.data());
        exit(1);
    }

    if (waitForExit) {
        waitpid(pid, nullptr, 0);
    }

    return pid;
}

int main() {
    string envVarValue = "sleep";
    setenv("PROC_TO_KILL", envVarValue.c_str(), 1);

    pid_t sleepPid = StartProcess("sleep 1000");
    pid_t catPid = StartProcess("cat");
    pid_t tailPid = StartProcess("tail -f /dev/null");

    cout << "Starting processes" << endl;
    sleep(100);

    bool sleepBefore = IsProcessRunning("sleep");
    bool catBefore = IsProcessRunning("cat");
    bool tailBefore = IsProcessRunningById(tailPid);

    cout << "Before Killer:" << endl;
    cout << "sleep: " << (sleepBefore ? "Running" : "Not Running") << endl;
    cout << "cat:      " << (catBefore ? "Running" : "Not Running") << endl;
    cout << "tail:  " << (tailBefore ? "Running" : "Not Running") << endl;

    string killerCmd = "./Killer --name cat --id " + to_string(tailPid);
    StartProcess(killerCmd, true);

    sleep(100);

    bool sleepAfter = IsProcessRunningById(sleepPid);
    bool catAfter = IsProcessRunningById(catPid);
    bool tailAfter = IsProcessRunningById(tailPid);

    cout << endl << "After Killer:" << endl;
    cout << "sleep (env var): " << (!sleepAfter ? "Killed" : "Still Running") << endl;
    cout << "cat (--name):    " << (!catAfter ? "Killed" : "Still Running") << endl;
    cout << "tail (--id):     " << (!tailAfter ? "Killed" : "Still Running") << endl;

    unsetenv("PROC_TO_KILL");

    if (sleepAfter) kill(sleepPid, SIGKILL);
    if (catAfter) kill(catPid, SIGKILL);
    if (tailAfter) kill(tailPid, SIGKILL);

    getchar();
    return 0;
}