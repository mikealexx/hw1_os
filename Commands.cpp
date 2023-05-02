#include "Commands.h"

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "signals.h"
#include <fcntl.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const int MAX_PATH = 256;

#if 0
#define FUNC_ENTRY() cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char*)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        ++i;
    }
    args[i] = NULL;
    return i;

    FUNC_EXIT()
}

void _parse_delete(char** args, int num_args)
{
	for (int i = 0; i < num_args; i++) {
		free(args[i]);
	}
	//free(args);
}

std::string _cmd_to_string(char** args) {
  std::string result;
  int i = 0;
  while (args[i] != nullptr) {  // loop over all non-null elements of args
    if (i > 0) {
      result += ' ';  // add a space between words, except for the first word
    }
    result += args[i];
    ++i;
  }
  return result;
}

bool _isBackgroundComamnd(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing
    // spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

std::string firstWord(const char* sentence) {
    // Find the start of the first non-space character in the sentence
    const char* start = sentence;
    while (*start == ' ') {
        start++;
    }

    // Find the end of the first word in the sentence
    const char* end = start;
    while (*end != '\0' && *end != ' ') {
        end++;
    }

    // Copy the first word into a string and return it
    return std::string(start, end - start);
}

bool containsWildcard(char* str) {
    if(str == nullptr || *str == '\0') {
        return false; // handle null or empty string cases
    }
    
    while(*str != '\0') {
        if(*str == '*' || *str == '?') {
            return true; // return true if '*' or '?' is found
        }
        str++; // move to the next character
    }
    
    return false; // return false if neither '*' nor '?' is found
}

bool _isRedirection(const char* cmd_line) {
  // Look for the ">" and ">>" operators in the command line string
  const char* op1 = std::strstr(cmd_line, ">");
  const char* op2 = std::strstr(cmd_line, ">>");

  // If either operator was found, return true
  return (op1 != nullptr || op2 != nullptr);
}

void _splitRedirectionPipe(const char* cmd_line, char* before, char* after) {
    const char* redirection = strpbrk(cmd_line, ">|");
    if (redirection == nullptr) {
        return; // no redirection found
    }

    size_t index = redirection - cmd_line;
    if (redirection[0] == '|' && redirection[1] == '&') {
        strncpy(before, cmd_line, index + 2);
        before[index + 2] = '\0';
        strcpy(after, redirection + 2);
    } else {
        strncpy(before, cmd_line, index);
        before[index] = '\0';
        strcpy(after, redirection + 1);
    }
}


// TODO: Add your implementation for classes in Commands.h

Command::Command(const char* cmd_line)
    : cmd_line(cmd_line) {}

Command::~Command() {};

BuiltInCommand::BuiltInCommand(const char* cmd_line)
    : Command(cmd_line) {}

//===================================================================
//======================== Built In Commands ========================
//===================================================================

string SmallShell::prompt = "smash";
string SmallShell::lastWd = "";
JobsList* SmallShell::jobs_list = new JobsList();
pid_t SmallShell::curr_pid = -1;
string empty = "";
char* SmallShell::curr_cmd_line = const_cast<char*>(empty.c_str());

ChpromptCommand::ChpromptCommand(const char* cmd_line)
    : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    char** args = (char**)malloc(COMMAND_MAX_ARGS * sizeof(char*));
    int args_num = _parseCommandLine(this->cmd_line, args);
    if (args_num > 1) {
        SmallShell::prompt = args[1];
    } else {
        SmallShell::prompt = "smash";
    }
    _parse_delete(args, args_num);
    free(args);
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line)
    : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char buffer[MAX_PATH];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        cout << buffer << endl;
    } else {
        perror("smash error: getcwd failed");
    }
}

ShowPidCommand::ShowPidCommand(const char* cmd_line)
    : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line)
    : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    if (args_num <= 1) {
        cerr << "smash error:> \"" << this->cmd_line << "\"" << endl;
        return;
    }
    char buffer[MAX_PATH];  // get current working directory
    if (getcwd(buffer, sizeof(buffer)) == nullptr) {
        perror("smash error: getcwd failed");
        return;
    }
    if (args_num > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
    } else if (strcmp(args[1], "-") == 0) {  //'cd -' was called
        if (SmallShell::lastWd == "") {      // last working directory is empty
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        } else if (chdir(SmallShell::lastWd.c_str()) == 0) {  // chdir succeeded
            SmallShell::lastWd = string(buffer);
        } else {  // chdir failed
            perror("smash error: chdir failed");
        }
    } else if (chdir(args[1]) == 0) {  // chdir succeeded
        SmallShell::lastWd = string(buffer);
    } else {
        //cout << "changing to: " << args[1] << endl;
        perror("smash error: chdir failed");
    }
    _parse_delete(args, args_num);
    free(args);
}

JobsCommand::JobsCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void JobsCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->printJobsList();
}

QuitCommand::QuitCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    if(args_num >= 2 && strcmp(args[1], "kill") == 0) {
        SmallShell::jobs_list->killAllJobs();
    }
    _parse_delete(args, args_num);
    free(args);
    exit(0);
}

ForegroundCommand::ForegroundCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    if(args_num > 2) {
        cerr << "smash error: fg: invalid arguments" << endl;
    }
    if(smash.jobs_list->empty()) { //jobs list is empty
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
    }
    int job_id;
    if(args_num == 1) {
        job_id = smash.jobs_list->max_job_id;
    }
    else if(atoi(args[1]) != 0){
        job_id = atoi(args[1]);
        if(smash.jobs_list->getJobById(job_id) == nullptr) {
            cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
        }
    }
    else if(atoi(args[1]) <= 0){
        cerr << "smash error: fg: invalid arguments" << endl;
    }
    //const char* cmd = ; //original cmd_line
    char* og_cmd = strdup(smash.jobs_list->getJobById(job_id)->og_cmd_line);
    pid_t fg_pid = smash.jobs_list->getJobById(job_id)->pid; //process pid
    smash.jobs_list->removeJobById(job_id);
    cout << og_cmd << " : " << fg_pid << endl;
    smash.curr_pid = fg_pid;
    smash.curr_cmd_line = og_cmd;
    if(kill(fg_pid, SIGCONT) < 0) {
        perror("smash error: kill failed");
    }
    int status;
    if(waitpid(fg_pid, &status, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
    }
    _parse_delete(args, args_num);
    free(args);
}

BackgroundCommand::BackgroundCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    if(args_num > 2) {
        cerr << "smash error: fg: invalid arguments" << endl;
    }
    int job_id;
    if(args_num == 1) {
        if(smash.jobs_list->getLastStoppedJob() == nullptr) {
            cerr << "smash error: bg: there is not stopped jobs to resume" << endl;
            exit(0);
        }
        job_id = smash.jobs_list->getJobIdByPid(smash.jobs_list->getLastStoppedJob()->pid);
        if(job_id == -1) {
            cerr << "smash error: bg: there is not stopped jobs to resume" << endl;
        }
    }
    else if(atoi(args[1]) != 0){
        job_id = atoi(args[1]);
        if(smash.jobs_list->getJobById(job_id) == nullptr) {
            cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
        }
        if(!smash.jobs_list->getJobById(job_id)->stopped) {
            cerr << "smash error: bg: job-id " << job_id << " is already running in the background";
        }
    }
    else if(atoi(args[1]) <= 0){
        cerr << "smash error: fg: invalid arguments" << endl;
    }
    smash.jobs_list->getJobById(job_id)->stopped = false;
    if(kill(smash.jobs_list->getJobById(job_id)->pid, SIGCONT) < 0) {
        perror("smash error: kill failed");
    }
    _parse_delete(args, args_num);
    free(args);
}

KillCommand::KillCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    if(args_num != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
    if(atoi(args[2]) <= 0 || atoi(args[1]) > 0) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
    int job_id = atoi(args[2]);
    cout << "signal number " << -(atoi(args[1])) << " was sent to pid " <<smash.jobs_list->getJobById(job_id)->pid << endl;
    if(kill(smash.jobs_list->getJobById(job_id)->pid, -(atoi(args[1]))) < 0) {
        perror("smash error: kill failed");
    }
    if(-(atoi(args[1])) == SIGSTOP) {
        smash.jobs_list->getJobById(job_id)->stopped = true;
    }
    else if(-(atoi(args[1])) == SIGCONT) {
        smash.jobs_list->getJobById(job_id)->stopped = false;
    }
    _parse_delete(args, args_num);
    free(args);
}

//===================================================================
//======================== External Commands ========================
//===================================================================

void append_nullptr(char** arr, size_t size) {
    arr[size] = nullptr;
}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    bool background = _isBackgroundComamnd(cmd_line);
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    char new_cmd_line[COMMAND_ARGS_MAX_LENGTH]; //cmd_line without '&'
    strcpy(new_cmd_line, cmd_line);
    char* og_cmd_line = strdup(cmd_line);
    _removeBackgroundSign(new_cmd_line);
    int args_num = _parseCommandLine(new_cmd_line, args);
    append_nullptr(args, args_num);
    int status;
    pid_t pid = fork();
    if(pid < 0) {
        perror("smash error: pid failed");
    }
    if(pid == 0) { //child process
        setpgrp();
        if(containsWildcard(new_cmd_line)) {
            string bash = "bash";
            string flag = "-c";
            char* const argv[] = {(char* const)bash.c_str(), (char* const)flag.c_str(), new_cmd_line, nullptr};
            if(execv("/bin/bash", argv) < 0) {
                perror("smash error: execv failed");
            }
        }
        else {
            if(execvp(args[0], args) < 0) {
                perror("smash error: execvp failed");
            }
        }
    }
    else { //parent process
        if(!background) {
            smash.curr_pid = pid;
            smash.curr_cmd_line = og_cmd_line;
            //ctrlZHandler(23);
            if(waitpid(pid, &status, WUNTRACED) < 0) {
                perror("smash error: wait failed");
            }
            smash.curr_pid = -1;
        }
        else {
            smash.jobs_list->removeFinishedJobs();
            JobsList::JobEntry* job = new JobsList::JobEntry(pid, og_cmd_line, time(nullptr), false);
            smash.jobs_list->addJob(pid, job);
        }
    }
    _parse_delete(args, args_num);
    free(args);
}

//=====================================================================
//======================== Redirection & Pipes ========================
//=====================================================================

RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line) {}

void RedirectionCommand::execute() {
    char** args = (char**)malloc(sizeof(char*)* COMMAND_MAX_ARGS);
    char new_cmd_line[COMMAND_ARGS_MAX_LENGTH]; //cmd_line without '&'
    int args_num = _parseCommandLine(new_cmd_line, args);
    strcpy(new_cmd_line, cmd_line);
    string cmd_str = new_cmd_line;
    string first_cmd;
    string second_cmd;
    int first_index = cmd_str.find_first_of(">");
	int last_index = cmd_str.find_last_of(">");
	first_cmd = cmd_str.substr(0, first_index);
	second_cmd = cmd_str.substr(last_index + 1);
	first_cmd = _trim(first_cmd);
	second_cmd = _trim(second_cmd);
    int fd;
    int out = dup(1);
    if(out < 1) {
        perror("smash error: dup failed");
    }
    if(close(1) < 0) {
        perror("smash error: close failed");
    }
    if(first_index == last_index) { // "cmd1 > cmd2" - write over
        fd = open((char*)second_cmd.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
        if(fd < 0) {
            perror("smash error: open failed");
            dup2(out,1);
        }

    }
    else { // "cmd1 >> cmd2" - append
        fd = open((char*)second_cmd.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);
        if(fd < 0) {
            perror("smash error: open failed");
            dup2(out,1);
        }
    }
    string command = firstWord(first_cmd.c_str());
    if (command == "chprompt") {
        ChpromptCommand chprompt(first_cmd.c_str());
        chprompt.execute();
    } else if (command == "showpid") {
        ShowPidCommand showpid(first_cmd.c_str());
        showpid.execute();
    } else if (command == "pwd") {
        GetCurrDirCommand pwd(first_cmd.c_str());
        pwd.execute();
    } else if (command == "cd") {
        ChangeDirCommand cd(first_cmd.c_str());
        cd.execute();
    }
    else if(command == "quit") {
        QuitCommand quit(first_cmd.c_str());
        quit.execute();
    }
    else if(command == "jobs") {
        JobsCommand jobs(first_cmd.c_str());
        jobs.execute();
    }
    else if(command == "fg") {
        ForegroundCommand fg(first_cmd.c_str());
        fg.execute();
    }
    else if(command == "bg") {
        BackgroundCommand bg(first_cmd.c_str());
        bg.execute();
    }
    else if(command == "kill") {
        KillCommand kill_cmd(first_cmd.c_str());
        kill_cmd.execute();
    }
    else {
        ExternalCommand external(first_cmd.c_str());
        external.execute();
    }
    close(fd);
    dup2(out,1);
    _parse_delete(args, args_num);
    free(args);
}

//==============================================================
//======================== Jobs Classes ========================
//==============================================================

JobsList::JobsList(): jobs_map(), max_job_id(0) {}

JobsList::~JobsList() {
    for(auto const& entry : this->jobs_map) {
        delete entry.second;
    }
}

void JobsList::addJob(pid_t pid, JobEntry* job, bool isStopped) {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->jobs_map.insert(pair<int, JobEntry*>(smash.jobs_list->max_job_id+1, job));
    smash.jobs_list->max_job_id++;
}

void JobsList::printJobsList() {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    if(smash.jobs_list->jobs_map.empty()) {
        return;
    }
    for(auto const& entry : smash.jobs_list->jobs_map) {
        cout << "[" << entry.first << "] " << entry.second->og_cmd_line << " : " << entry.second->pid << " " << difftime(time(nullptr), entry.second->start_time) << " secs";
        if(entry.second->stopped) {
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    cout << "sending SIGKILL signal to " << this->jobs_map.size() << " jobs:" << endl;
    if(this->jobs_map.empty()) {
        return;
    }
    for(auto const& entry : this->jobs_map) {
        cout << entry.second->pid << ": " << entry.second->og_cmd_line << endl;
        if(entry.second->og_cmd_line != nullptr) {
            free(entry.second->og_cmd_line);
        }
        if(kill(entry.second->pid, 9) < 0) {
            perror("smash error: kill failed");
        }
    }
}

void JobsList::removeFinishedJobs() {
    if (this->jobs_map.empty()) {
        return;
    }
    std::vector<int> finished_jobs;
    for (const auto& entry : this->jobs_map) {
        int status;
        pid_t result = waitpid(entry.second->pid, &status, WNOHANG);
        if (result == entry.second->pid) { // Process has finished
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                if(entry.second->og_cmd_line != nullptr) {
                    free(entry.second->og_cmd_line);
                }
                delete entry.second;
                finished_jobs.push_back(entry.first);
            }
        }
    }
    for (const auto& job_id : finished_jobs) {
        this->jobs_map.erase(job_id);
    }
    this->max_job_id = 0;
    for (const auto& entry : this->jobs_map) {
        if (entry.first > this->max_job_id) {
            this->max_job_id = entry.first;
        }
    }
}


JobsList::JobEntry* JobsList::getJobById(int jobId) {
    if(this->jobs_map.find(jobId) != this->jobs_map.end()) {
        return this->jobs_map.find(jobId)->second;
    }
    else {
        return nullptr;
    }
}

void JobsList::removeJobById(int jobId) {
    if(this->jobs_map.find(jobId) != this->jobs_map.end()) { //job exists in map
        free(this->jobs_map.find(jobId)->second->og_cmd_line);
        delete this->jobs_map.find(jobId)->second;
        this->jobs_map.erase(jobId);
        if(jobId == this->max_job_id) {
            this->max_job_id = 0;
            for(auto const& entry : this->jobs_map) { //updating max job id
                if(entry.first > this->max_job_id) {
                    this->max_job_id = entry.first;
                }
            }
        }
    }
}

bool JobsList::empty() {
    return this->jobs_map.empty();
}

JobsList::JobEntry* JobsList::getJobByPid(pid_t pid) {
    if(this->jobs_map.empty()) {
        return nullptr;
    }
    for(const auto& entry : this->jobs_map) {
        if(entry.second->pid == pid) {
            return entry.second; 
        }
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastStoppedJob() {
    if(this->jobs_map.empty()) {
        return nullptr;
    }
    for(auto it = this->jobs_map.rbegin(); it != this->jobs_map.rend(); ++it) {
        if(it->second->stopped) {
            return it->second;
        }
    }
    return nullptr;
}

int JobsList::getJobIdByPid(pid_t pid) {
    if(this->jobs_map.empty()) {
        return -1;
    }
    for(auto const& entry : this->jobs_map) {
        if(entry.second->pid == pid) {
            return entry.first;
        }
    }
    return -1;
}

// JobList::JobEntry* JobList::getLastJob(int* lastJobId) {

// }

SmallShell::SmallShell() {}

SmallShell::~SmallShell() {
    delete SmallShell::jobs_list;
} 

Command* SmallShell::CreateCommand(const char* cmd_line) {
    // For example:
    /*
      string cmd_s = _trim(string(cmd_line));
      string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

      if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
      }
      else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
      }
      else if ...
      .....
      else {
        return new ExternalCommand(cmd_line);
      }
      */
    return nullptr;
}

void SmallShell::executeCommand(const char* cmd_line) {
    string command = firstWord(cmd_line);
    if(_isRedirection(cmd_line)) {
        RedirectionCommand redirect(cmd_line);
        redirect.execute();
    }
    else if(command == "") {
        return;
    }
    else if (command == "chprompt") {
        ChpromptCommand chprompt(cmd_line);
        chprompt.execute();
    } else if (command == "showpid") {
        ShowPidCommand showpid(cmd_line);
        showpid.execute();
    } else if (command == "pwd") {
        GetCurrDirCommand pwd(cmd_line);
        pwd.execute();
    } else if (command == "cd") {
        ChangeDirCommand cd(cmd_line);
        cd.execute();
    }
    else if(command == "quit") {
        QuitCommand quit(cmd_line);
        quit.execute();
    }
    else if(command == "jobs") {
        JobsCommand jobs(cmd_line);
        jobs.execute();
    }
    else if(command == "fg") {
        ForegroundCommand fg(cmd_line);
        fg.execute();
    }
    else if(command == "bg") {
        BackgroundCommand bg(cmd_line);
        bg.execute();
    }
    else if(command == "kill") {
        KillCommand kill_cmd(cmd_line);
        kill_cmd.execute();
    }
    else {
        ExternalCommand external(cmd_line);
        external.execute();
    }
}