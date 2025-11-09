#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cstdlib>
#include <signal.h>
using namespace std;
struct ProcessInfo {
 int pid;
 string name;
 string user;
 long memory_kb;
};
struct SystemInfo {
 long mem_total_kb;
 long mem_free_kb;
};
bool is_number(const string& s) {
 for (char const &ch : s) {
 if (isdigit(ch) == 0) return false;
 }
 return !s.empty();
}
string get_username(const string& uid) {
 if (uid == "0") return "root";
 char* user = getenv("USER");
 if (user) return string(user);
 return "user";
}
SystemInfo get_system_info() {
 SystemInfo info = {0, 0};
 ifstream file("/proc/meminfo");
 string line;
 while (getline(file, line)) {
 stringstream ss(line);
 string key;
 long value;
 ss >> key >> value;
 if (key == "MemTotal:") {
 info.mem_total_kb = value;
 } else if (key == "MemAvailable:") {
 info.mem_free_kb = value;
 } else if (key == "MemFree:" && info.mem_free_kb == 0) {
 info.mem_free_kb = value;
 }
 }
 file.close();
 return info;
}
vector<ProcessInfo> get_process_list() {
 vector<ProcessInfo> processes;
 DIR* proc_dir = opendir("/proc");
 if (proc_dir == NULL) {
 perror("Could not open /proc");
 return processes;
 }
 struct dirent* entry;
 while ((entry = readdir(proc_dir)) != NULL) {
 if (is_number(entry->d_name)) {
 ProcessInfo p;
 p.pid = stoi(entry->d_name);
 string status_path = string("/proc/") + entry->d_name + "/status";
 ifstream status_file(status_path);
 string line;
 if (!status_file) continue;
 p.memory_kb = 0;
 string uid_str;
 while (getline(status_file, line)) {
 stringstream ss(line);
 string key;
 ss >> key;
 if (key == "Name:") {
 ss >> p.name;
 } else if (key == "Uid:") {
 ss >> uid_str;
 } else if (key == "VmRSS:") {
 ss >> p.memory_kb;
 }
 }
 status_file.close();
 p.user = get_username(uid_str);
 if (p.memory_kb > 0) {
 processes.push_back(p);
 }
 }
 }
 closedir(proc_dir);
 return processes;
}
bool compare_by_memory(const ProcessInfo& a, const ProcessInfo& b) {
 return a.memory_kb > b.memory_kb;
}
void display_dashboard(const SystemInfo& sys, vector<ProcessInfo>& proc_list) {
 system("clear");
 cout << "--- System Monitor Tool (LSP Capstone) ---" << endl << endl;
 float mem_used_mb = (sys.mem_total_kb - sys.mem_free_kb) / 1024.0;
 float mem_total_mb = sys.mem_total_kb / 1024.0;
 float mem_percent = (mem_used_mb / mem_total_mb) * 100.0;
 cout << fixed << setprecision(2);
 cout << "--- Process List (Sorted by Memory) ---" << endl;

 sort(proc_list.begin(), proc_list.end(), compare_by_memory);
 cout << left;
 cout << setw(10) << "PID"
 << setw(12) << "USER"
 << setw(25) << "NAME"
 << setw(15) << "MEMORY (MB)" << endl;
 cout << string(62, '-') << endl;
 int count = 0;
 for (const auto& p : proc_list) {
 if (count++ >= 15) break;
 cout << setw(10) << p.pid
 << setw(12) << p.user
 << setw(25) << p.name.substr(0, 24) // Truncate long names
 << setw(15) << (p.memory_kb / 1024.0) << endl;
 }

 cout << endl << "Refreshing in 3 seconds... (Press Ctrl+C to exit)" << endl;


 cout << "Enter PID to kill (or 0 to skip): ";
 int pid_to_kill;
 cin >> pid_to_kill;
 if (pid_to_kill > 0) {
 if (kill(pid_to_kill, SIGKILL) == 0) {
 cout << "Process " << pid_to_kill << " killed." << endl;
 } else {
 perror("Error killing process");
 }
 }
}
// --- Main Function ---
int main() {
 while (true) {
 SystemInfo sys_info = get_system_info();
 vector<ProcessInfo> proc_list = get_process_list();
 display_dashboard(sys_info, proc_list); 
 sleep(3);
 }
 return 0;
}