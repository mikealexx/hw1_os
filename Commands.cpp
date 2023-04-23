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
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    if(args_num > 2) {
        cerr << "smash error: fg: invalid arguments" << endl;
    }
    pid_t pid = fork();
    if(pid < 0) {
        perror("smash error: fork failed");
    }
    if(pid == 0) { //child
        setpgrp();
        if(args_num == 1) { //"fg"
            if(SmallShell::jobs_list->empty()) { //jobs list is empty
                cerr << "smash error: fg: jobs list is empty" << endl;
                return;
            }
            const char* cmd = SmallShell::jobs_list->getJobById(SmallShell::jobs_list->max_job_id)->og_cmd_line;
            pid_t fg_pid = SmallShell::jobs_list->getJobById(SmallShell::jobs_list->max_job_id)->pid;
            //cout << "remove job " << endl;
            SmallShell::jobs_list->removeJobById(SmallShell::jobs_list->max_job_id);
            cout << cmd << " : " << fg_pid << endl;
            SmallShell::curr_pid = fg_pid;
            if(SmallShell::jobs_list->getJobById(SmallShell::jobs_list->max_job_id)->stopped) {
                if(kill(SmallShell::jobs_list->getJobById(SmallShell::jobs_list->max_job_id)->pid, 18) < 0) {
                    perror("smash error: kill failed");
                }
            }
        }
        if(args_num == 2) { //"fg <job-id>"
            if(atoi(args[1]) == 0) {
                cerr << "smash error: fg: invalid arguments" << endl;
            }
            if(SmallShell::jobs_list->getJobById(atoi(args[1])) == nullptr) { //job id does not exist
                cerr << "smash error: fg: job-id " << args[1] << " does not exist" << endl;
            }
            const char* cmd = SmallShell::jobs_list->getJobById(atoi(args[1]))->og_cmd_line;
            pid_t fg_pid = SmallShell::jobs_list->getJobById(atoi(args[1]))->pid;
            SmallShell::jobs_list->removeJobById(atoi(args[1]));
            cout << cmd << " : " << fg_pid << endl;
            SmallShell::curr_pid = fg_pid;
            if(SmallShell::jobs_list->getJobById(atoi(args[1]))->stopped) {
                if(kill(SmallShell::jobs_list->getJobById(atoi(args[1]))->pid, 18) < 0) {
                    perror("smash error: kill failed");
                }
            }
        }
    }
    else {
        int status;
        if(waitpid(SmallShell::jobs_list->getJobById(SmallShell::jobs_list->max_job_id)->pid, &status, WUNTRACED) < 0) {
            perror("smash error: waitpid failed");
        }
        SmallShell::curr_pid = -1;
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
        if(execvp(args[0], args) < 0) {
            perror("smash error: execvp failed");
        }
    }
    else { //parent process
        if(!background) {
            smash.curr_pid = pid;
            if(wait(&status) < 0) {
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
            cout << "(stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    if(this->jobs_map.empty()) {
        return;
    }
    for(auto const& entry : this->jobs_map) {
        free(entry.second->og_cmd_line);
        if(kill(entry.second->pid, 9) < 0) {
            perror("smash error: kill failed");
        }
    }
}

/*
void JobsList::removeFinishedJobs() {
    if(this->jobs_map.empty()) {
        return;
    }
    for(auto it = this->jobs_map.begin(); it != this->jobs_map.end();) {
        pid_t curr_pid = it->second->pid;
        int status;
        pid_t result = waitpid(curr_pid, &status, WNOHANG);
        if(result == -1) { //finished process
            if(WIFEXITED(status) || WIFSIGNALED(status)) {
                free(it->second->og_cmd_line);
                delete it->second;
                it = this->jobs_map.erase(it);
            }
        }
        else {
            ++it;
        }
    }
    this->max_job_id = 0; //updating max job-id
    for(auto const& entry : this->jobs_map) {
        if(entry.first > this->max_job_id) {
            this->max_job_id = entry.first;
        }
    }
}
*/

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
                free(entry.second->og_cmd_line);
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

// JobList::JobEntry* JobList::getLastJob(int* lastJobId) {

// }

SmallShell::SmallShell() {}

SmallShell::~SmallShell() {
    delete SmallShell::jobs_list;
} 

/**
 * Creates and returns a pointer to Command class which matches the given
 * command line (cmd_line)
 */
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
    if(command == "") {
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
    else {
        ExternalCommand external(cmd_line);
        external.execute();
    }
}