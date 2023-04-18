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
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
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
    : cmd_line(cmd_line), args_num(0), args(nullptr) {
    this->args = (char**)malloc(COMMAND_MAX_ARGS);
    this->args_num = _parseCommandLine(this->cmd_line, this->args);
}

Command::~Command() {
    free(this->args);
};

BuiltInCommand::BuiltInCommand(const char* cmd_line)
    : Command(cmd_line) {}

//===================================================================
//======================== Built In Commands ========================
//===================================================================

string SmallShell::prompt = "smash";
string SmallShell::lastWd = "";
JobsList* SmallShell::jobs_list;

ChpromptCommand::ChpromptCommand(const char* cmd_line)
    : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    if (this->args_num > 1) {
        SmallShell::prompt = args[1];
    } else {
        SmallShell::prompt = "smash";
    }
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
    if (this->args_num <= 1) {
        cerr << "smash error:> \"" << this->cmd_line << "\"" << endl;
        return;
    }
    char buffer[MAX_PATH];  // get current working directory
    if (getcwd(buffer, sizeof(buffer)) == nullptr) {
        perror("smash error: getcwd failed");
        return;
    }
    if (this->args_num > 2) {
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
}

//===================================================================
//======================== External Commands ========================
//===================================================================

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute() {
    int stat;
    bool background = _isBackgroundComamnd(cmd_line);
    if(background) {
        char* non_const = const_cast<char*>(cmd_line);
        _removeBackgroundSign(non_const);
    }
    char* exe_args[] = {const_cast<char*>(this->cmd_line), nullptr};
    pid_t pid = fork();
    if(pid < 0) { //fork failed
        perror("smash error: fork failed");
    }
    else if(pid == 0) { //child process
        setpgrp();
        execvp(firstWord(args[0]).c_str(), exe_args);
    }
    else { //parent process
        if(wait(&stat) < 0) {
            perror("smash error: wait failed");
        }
    }
}

//==============================================================
//======================== Jobs Classes ========================
//==============================================================


JobsList::JobsList(): jobs_list(), max_job_id(0) {}

JobsList::~JobsList() {
    while(!this->jobs_list.empty()) {
        delete this->jobs_list.back();
        this->jobs_list.pop_back();
    }
}

void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped) {
    this->jobs_list.push_back(new JobEntry(cmd, this->max_job_id + 1, pid ,cmd->cmd_line, time(nullptr), isStopped));
    this->max_job_id++;
}

void JobsList::printJobsList() {
    for(list<JobEntry*>::iterator it = this->jobs_list.begin(); it != this->jobs_list.end(); ++it) {
        cout << "[" << (*it)->job_id << "] " << (*it)->cmd_line << " : " << (*it)->pid << " " << difftime(time(nullptr), (*it)->start_time);
        if((*it)->stopped) {
            cout << "(stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    this->jobs_list.sort(JobsList::comparePid);
    cout << "smash: sending SIGKILL signal to " << this->jobs_list.size() << " jobs:" << endl;
    for(list<JobEntry*>::iterator it = this->jobs_list.begin(); it != this->jobs_list.end(); ++it) {
        cout << (*it)->pid << ": " << (*it)->cmd_line << endl;
    }
}

void JobsList::removeFinishedJobs() {
    for(list<JobEntry*>::iterator it = this->jobs_list.begin(); it != this->jobs_list.end(); ) {
        if(kill((*it)->pid, 0) != 0) {
            it = this->jobs_list.erase(it);
        } else {
            ++it;
        }
    }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for(list<JobEntry*>::iterator it = this->jobs_list.begin(); it != this->jobs_list.end(); ++it) {
        if((*it)->job_id == jobId) {
            return (*it);
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    JobsList::JobEntry* job = this->getJobById(jobId);
    if(job != nullptr) {
        this->jobs_list.remove(job);
    }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
    if(!this->jobs_list.empty()) {
        return this->jobs_list.back();
    }
    return nullptr;
}

JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {
    for(list<JobEntry*>::iterator it = this->jobs_list.rbegin(); it != this->jobs_list.rend(); ++it) {
        if((*it)->stopped) {
            return (*it);
        }
    }
    return nullptr;
}

SmallShell::SmallShell() {}

SmallShell::~SmallShell() {
    // TODO: add your implementation
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
    if (command == "chprompt") {
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
    else {
        ExternalCommand external(cmd_line);
        external.execute();
    }
}