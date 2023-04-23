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
    SmallShell::jobs_list->printJobsList();
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
            const char* cmd = SmallShell::jobs_list->getJobById(SmallShell::jobs_list->max_job_id)->cmd_line;
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
            const char* cmd = SmallShell::jobs_list->getJobById(atoi(args[1]))->cmd_line;
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

void append_null_terminator(char** arr, int size) {
    int i = 0;
    while (i < size && arr[i] != nullptr) {
        i++;
    }
    if (i < size) {
        arr[i] = nullptr;
    }
}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute() {
    /*
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    char* og_cmd_line = (char*)malloc(strlen(cmd_line) + 1);
    strcpy(og_cmd_line, cmd_line);
    int stat;
    bool background = _isBackgroundComamnd(cmd_line);
    char* non_const = const_cast<char*>(cmd_line);
    _removeBackgroundSign(non_const);
    append_null_terminator(args, args_num);
    //char* exe_args[] = {nullptr};
    pid_t pid = fork();
    if(pid < 0) { //fork failed
        perror("smash error: fork failed");
    }
    else if(pid == 0) { //child process
        setpgrp();
        //cout << args[0] << " " << args[1] << endl;
        //execvp(firstWord(args[0]).c_str(), exe_args);

        cout << "executing command: ";
        for (int i = 0; args[i] != nullptr; i++) {
            cout << args[i] << " ";
        }
        cout << endl;

        execvp(args[0], (char *const *) &non_const);
    }
    else { //parent process
        if(background) {
            //cout << og_cmd_line << endl;
            SmallShell::jobs_list->addJob(pid, og_cmd_line, false);
        }
        else {
            SmallShell::curr_pid = pid;
            if(waitpid(pid, &stat, WUNTRACED) < 0) {
                perror("smash error: wait failed");
            }
            SmallShell::curr_pid = -1;
        }
    }
    _parse_delete(args, args_num);
    free(args);
    */

    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(this->cmd_line, args);
    bool background = _isBackgroundComamnd(cmd_line);
    char new_cmd_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(new_cmd_line, cmd_line);
    char place[10] = "bash";
	char flag[4] = "-c";
	_removeBackgroundSign(new_cmd_line);
    char* const argv[] = { place, flag, new_cmd_line, nullptr};
    pid_t pid = fork();
    if(pid < 0) { //fork failed
        perror("smash error: fork failed");
    }
    if(pid == 0) { //child process
        setpgrp();
        if (execv("/bin/bash", argv) == -1)
		{
			perror("smash error: execv failed");
			exit(0);
		}
        exit(0);
    }
    else { //parent process
        if(!background) { //with wait
            SmallShell::curr_pid = pid;
            int status;
			waitpid(pid, &status, WUNTRACED);
            SmallShell::curr_pid = -1;
        }
        else { //no wait - add to jobs list

        }
    }
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

void JobsList::addJob(pid_t pid, const char* cmd_line, bool isStopped) {
    this->jobs_map.insert(pair<int, JobEntry*>(this->max_job_id+1, new JobEntry(pid, cmd_line, time(nullptr), isStopped)));
    this->max_job_id++;
}

void JobsList::printJobsList() {
    for(auto const& entry : this->jobs_map) {
        cout << "[" << entry.first << "] " << entry.second->cmd_line << " : " << entry.second->pid << " " << difftime(time(nullptr), entry.second->start_time);
        if(kill(entry.second->pid, 0) != 0) { //check if process is stopped
            cout << " (stopped)";
        }
        cout << endl;
    }
}

void JobsList::killAllJobs() {
    if(this->jobs_map.empty()) {
        return;
    }
    for(auto const& entry : this->jobs_map) {
        if(kill(entry.second->pid, 9) < 0) {
            perror("smash error: kill failed");
        }
    }
}

void JobsList::removeFinishedJobs() {
    for(auto const& entry : this->jobs_map) {
        if(kill(entry.second->pid, 0) == -1) { //find if current process has finished running
            if(errno == ESRCH) { //process doesn't exist as it has finished running
                int curr_job_id = entry.first;
                delete entry.second; //deallocation
                this->jobs_map.erase(curr_job_id); //erase the finished job from the map
                if(curr_job_id == this->max_job_id) { //we need to update the max job id
                    this->max_job_id = 0;
                    for(auto const& entry : this->jobs_map) {
                        if(entry.first > this->max_job_id) { //updating the max job id
                            this->max_job_id = entry.first;
                        }
                    }
                }
            }
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