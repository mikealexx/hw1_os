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
#include <sys/stat.h>
#include <bitset>

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

int _parseExternalCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()

    std::string cmd(cmd_line);
    std::vector<std::string> tokens;
    std::string current_token;

    bool in_quote = false;
    char quote_type;

    for (size_t i = 0; i < cmd.length(); i++) {
        char c = cmd[i];
        if (c == '"' || c == '\'') {
            if (!in_quote) {
                in_quote = true;
                quote_type = c;
            } else if (c == quote_type) {
                in_quote = false;
            } else {
                current_token += c;
            }
        } else if (c == ' ' && !in_quote) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token += c;
        }
    }

    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    int num_args = tokens.size();

    for (int i = 0; i < num_args; i++) {
        std::string& token = tokens[i];
        args[i] = (char*)malloc(token.length() + 1);
        strcpy(args[i], token.c_str());
    }

    args[num_args] = NULL;

    FUNC_EXIT()
    return num_args;
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

bool _isPipe(const char* cmd_line) {
    const std::string command = cmd_line;
    return command.find("|&") != std::string::npos || command.find("|") != std::string::npos;
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

std::string _parseTimeoutCommand(std::string cmd_line, int& duration) {
    // Find the index of the first space after "timeout"
    std::size_t timeout_index = cmd_line.find("timeout") + std::string("timeout").length();
    std::size_t duration_index = cmd_line.find_first_not_of(" ", timeout_index);
    
    // Extract the duration string and convert it to an integer
    std::string duration_string = cmd_line.substr(duration_index, cmd_line.size() - duration_index);
    std::stringstream ss(duration_string);
    ss >> duration;
    
    // Find the start of the command string and return it with leading/trailing spaces trimmed
    std::size_t command_index = cmd_line.find(to_string(duration)) + to_string(duration).size() + 1;
    return cmd_line.substr(command_index);
}

bool _isNumber(char* num) {
    if(num == nullptr) {
        return false;
    }
    for (int i=0; num[i]; i++) {
        if(num[i] == '-') {
            continue;
        }
        if (!isdigit(num[i])) {
            return false;
            break;
        }
    } 
    return true;
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
AlarmList* SmallShell::alarm_list = new AlarmList();
pid_t SmallShell::curr_pid = -1;
bool SmallShell::fg = false;
string empty = "";
char* SmallShell::curr_cmd_line = const_cast<char*>(empty.c_str());

ChpromptCommand::ChpromptCommand(const char* cmd_line)
    : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    char** args = (char**)malloc(COMMAND_MAX_ARGS * sizeof(char*));
    int args_num = _parseCommandLine(cmd_line, args);
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
        return;
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
    int args_num = _parseCommandLine(cmd_line, args);
    if (args_num <= 1) {
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    char buffer[MAX_PATH];  // get current working directory
    if (getcwd(buffer, sizeof(buffer)) == nullptr) {
        perror("smash error: getcwd failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if (args_num > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    } else if (strcmp(args[1], "-") == 0) {  //'cd -' was called
        if (SmallShell::lastWd == "") {      // last working directory is empty
            cerr << "smash error: cd: OLDPWD not set" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        } else if (chdir(SmallShell::lastWd.c_str()) == 0) {  // chdir succeeded
            SmallShell::lastWd = string(buffer);
        } else {  // chdir failed
            perror("smash error: chdir failed");
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    } else if (chdir(args[1]) == 0) {  // chdir succeeded
        SmallShell::lastWd = string(buffer);
    } else {
        //cout << "changing to: " << args[1] << endl;
        perror("smash error: chdir failed");
        _parse_delete(args, args_num);
        free(args);
        return;
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
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    smash.jobs_list->removeFinishedJobs();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
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
    smash.jobs_list->removeFinishedJobs();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
    int job_id;
    if(args_num == 1) { //no job-id specified
        job_id = smash.jobs_list->max_job_id;
        if(smash.jobs_list->empty()) { //jobs list is empty
            cerr << "smash error: fg: jobs list is empty" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    } 
    else if(atoi(args[1]) != 0){
        if(args_num > 2) {
            cerr << "smash error: fg: invalid arguments" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        job_id = atoi(args[1]);
        if(smash.jobs_list->getJobById(job_id) == nullptr) {
            cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    }
    else if(atoi(args[1]) <= 0){
        cerr << "smash error: fg: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(args_num > 2) {
        cerr << "smash error: fg: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    //const char* cmd = ; //original cmd_line
    char* og_cmd = strdup(smash.jobs_list->getJobById(job_id)->og_cmd_line);
    pid_t fg_pid = smash.jobs_list->getJobById(job_id)->pid; //process pid
    //smash.jobs_list->removeJobById(job_id);
    //smash.jobs_list->getJobById(job_id)->fg = true;
    cout << og_cmd << " : " << fg_pid << endl;
    smash.curr_pid = fg_pid;
    smash.curr_cmd_line = og_cmd;
    if(kill(fg_pid, SIGCONT) < 0) {
        perror("smash error: kill failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    int status;
    if(waitpid(fg_pid, &status, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    free(og_cmd);
    smash.curr_pid = -1;
    _parse_delete(args, args_num);
    free(args);
}

BackgroundCommand::BackgroundCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void BackgroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
    int job_id;
    if(args_num == 1) {
        if(smash.jobs_list->getLastStoppedJob() == nullptr) {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        job_id = smash.jobs_list->getJobIdByPid(smash.jobs_list->getLastStoppedJob()->pid);
        if(job_id == -1) {
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    }
    else if(atoi(args[1]) != 0){
        if(args_num > 2) {
            cerr << "smash error: bg: invalid arguments" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        job_id = atoi(args[1]);
        if(smash.jobs_list->getJobById(job_id) == nullptr) {
            cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        if(!smash.jobs_list->getJobById(job_id)->stopped) {
            cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    }
    else if(atoi(args[1]) <= 0){
        cerr << "smash error: bg: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(args_num > 2) {
        cerr << "smash error: bg: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    smash.jobs_list->getJobById(job_id)->stopped = false;
    char* og_cmd = strdup(smash.jobs_list->getJobById(job_id)->og_cmd_line);
    pid_t fg_pid = smash.jobs_list->getJobById(job_id)->pid; //process pid
    cout << og_cmd << " : " << fg_pid << endl;
    if(kill(smash.jobs_list->getJobById(job_id)->pid, SIGCONT) < 0) {
        perror("smash error: kill failed");
        _parse_delete(args, args_num);
        free(og_cmd);
        free(args);
        return;
    }
    free(og_cmd);
    _parse_delete(args, args_num);
    free(args);
}

SetcoreCommand::SetcoreCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void SetcoreCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    /*
        1) job-id not a number, core not a number - invalid arguments
        2) job-id not a number, core is a number - invalid arguments
        3) job-id is a number, core not a number - check if job-id valid, if not - invalid job-id,
                                              else - invalid arguments
        4) job-id is a number, core is a number - check if job-id is valid, if not - invalid job-id,
                                                    else, check if core valid.
    */
    if(!_isNumber(args[1])) { // job-id not a number
        cerr << "smash error: setcore: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    else if(!_isNumber(args[2])) { // job-id number, core not a number
        if(smash.jobs_list->getJobById(atoi(args[1])) == nullptr) { // job-id invalid
            cerr << "smash error: setcore: job-id " << atoi(args[1]) << " does not exist" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        cerr << "smash error: setcore: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    else { // job-id number, core number
        if(smash.jobs_list->getJobById(atoi(args[1])) == nullptr) { // invalid job-id
            cerr << "smash error: setcore: job-id " << atoi(args[1]) << " does not exist" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        else if(atoi(args[2]) < 0 || atoi(args[2]) > num_cores) { // invalid core number
            cerr << "smash error: setcore: invalid core number" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    }
    if(args_num != 3) {
        cerr << "smash error: setcore: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    int job_id = atoi(args[1]);
    pid_t fg_pid = smash.jobs_list->getJobById(job_id)->pid; //process pid
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(atoi(args[2]), &cpuset);
    if(sched_setaffinity(fg_pid, sizeof(cpu_set_t), &cpuset) < 0) {
        perror("smash error: sched_setaffinity failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    _parse_delete(args, args_num);
    free(args);
}

KillCommand::KillCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
    /*
        1) job-id not a number, signal not a number - invalid arguments
        2) job-id not a number, signal number - invalid arguments
        3) job-id number, signal not a number - check if job-id valid, if not - invalid job-id,
                                                else - invalid arguments
        4) job-id number, signal number - check if job-id valid, if not - invalid job-id,
                                                else - let kill fail
    
    */
    if(!_isNumber(args[2])) { // job-id not a number
        cerr << "smash error: kill: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    else if(!_isNumber(args[1])) { // job-id is a number, signal not a number
        if(smash.jobs_list->getJobById(atoi(args[2])) == nullptr) { // job doesn't exist
            cerr << "smash error: kill: job-id " << atoi(args[2]) << " does not exist" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        cerr << "smash error: kill: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    else { // job-id is a number, signal is a number
        if(smash.jobs_list->getJobById(atoi(args[2])) == nullptr) { // job doesn't exist
            cerr << "smash error: kill: job-id " << atoi(args[2]) << " does not exist" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
        if(atoi(args[1]) >= 0) { // invalid signal number
            cerr << "smash error: kill: invalid arguments" << endl;
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    }
    if(args_num != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    int job_id = atoi(args[2]);
    int sig = -(atoi(args[1]));
    if(kill(smash.jobs_list->getJobById(job_id)->pid, sig) < 0) {
        perror("smash error: kill failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    cout << "signal number " << -(atoi(args[1])) << " was sent to pid " <<smash.jobs_list->getJobById(job_id)->pid << endl;
    if(-(atoi(args[1])) == SIGSTOP) {
        smash.jobs_list->getJobById(job_id)->stopped = true;
    }
    else if(-(atoi(args[1])) == SIGCONT) {
        smash.jobs_list->getJobById(job_id)->stopped = false;
    }
    _parse_delete(args, args_num);
    free(args);
    smash.jobs_list->removeFinishedJobs();
}

GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void GetFileTypeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
    if(args_num != 2) {
        cerr << "smash error: gettype: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    string file = (string)args[1];
    struct stat file_stat;
    string type;
    if(stat(file.c_str(), &file_stat) < 0) {
        perror("smash error: stat failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    switch(file_stat.st_mode & S_IFMT) {
        case S_IFREG:
            type = "regular type";
            break;
        case S_IFDIR:
            type = "directory";
            break;
        case S_IFCHR:
            type = "character device";
            break;
        case S_IFBLK:
            type = "block device";
            break;
        case S_IFIFO:
            type = "FIFO";
            break;
        case S_IFLNK:
            type = "symbolic link";
            break;
        case S_IFSOCK:
            type = "socket";
            break;
    }
    off_t size = file_stat.st_size;
    cout << args[1] << "\'s type is " << type << " and takes up " << size << " bytes" << endl;
    _parse_delete(args, args_num);
    free(args);
}

ChmodCommand::ChmodCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ChmodCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    smash.jobs_list->removeFinishedJobs();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    int args_num = _parseCommandLine(cmd_line, args);
    if(args_num != 3) {
        cerr << "smash error: chmod: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    mode_t mode = stoi(args[1], 0, 8);
    if(chmod(args[2], mode) < 0) {
        perror("smash error: chmod failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    _parse_delete(args, args_num);
    free(args);
}

void removeAmpersand(std::string& str) {
    if (str.back() == '&') {
        str.pop_back();
    }
}

int countWords(const char* str) {
    std::vector<std::string> words;
    std::stringstream ss(str);
    std::string word;
    while (ss >> word) {
        words.push_back(word);
    }
    return words.size();
}

TimeoutCommand::TimeoutCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void TimeoutCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    bool background = _isBackgroundComamnd(cmd_line);
    smash.jobs_list->removeFinishedJobs();
    char** args = (char**)malloc(sizeof(char*) * COMMAND_MAX_ARGS);
    string og_cmd_line = (string)cmd_line;
    int duration;
    string new_cmd_line;
    int words = countWords(cmd_line);
    if(words < 3) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        free(args);
        return;
    }
    try {
        new_cmd_line = _parseTimeoutCommand((string)cmd_line, duration);
    }
    catch(const std::invalid_argument&) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        free(args);
        return;
    }
    int args_num = _parseCommandLine(new_cmd_line.c_str(), args);
    if(duration <= 0) { //invalid timeout duration
        cerr << "smash error: timeout: invalid arguments" << endl;
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    int i = 0;
	std::string::const_iterator it = new_cmd_line.begin();
	while (it != new_cmd_line.end())
	{
		new_cmd_line[i] = *it;
		i++;
		it++;
	}
	new_cmd_line[i] = '\0';
	char place[10] = "/bin/bash";
	char flag[4] = "-c";
    char* with_ampersand = strdup(cmd_line);
	removeAmpersand(new_cmd_line);
	char* const argv[] = {place, flag, (char* const)new_cmd_line.c_str(), nullptr};
	pid_t pid = fork();
    if(pid < 0) {
        perror("smash error: pid failed");
        _parse_delete(args, args_num);
        free(args);
        free(with_ampersand);
        return;
    }
    if(pid == 0) { //child process
        setpgrp();
        signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		if (execv("/bin/bash", argv) == -1)
		{
			std::cerr << "smash error: execv failed" << std::endl;
            _parse_delete(args, args_num);
            free(args);
            free(with_ampersand);
			exit(0);
		}
    }
    else { //parent process
        time_t curr_time = time(nullptr);
        smash.alarm_list->addAlarm(og_cmd_line, curr_time, duration, pid);
        int time_to_alarm = smash.alarm_list->lowestTime(curr_time);
        alarm(time_to_alarm); //set alarm
        if(!background) {
            smash.fg = true;
            smash.curr_pid = pid;
            int status;
            if(waitpid(pid, &status, WUNTRACED) < 0) {
                perror("smash error: waitpid failed");
                _parse_delete(args, args_num);
                free(args);
                return;
            } //if we got here - job finished before the alarm
            smash.alarm_list->removeAlarmByPid(pid);
            free(with_ampersand);
            smash.curr_pid = -1;
            smash.fg = false;
        }
        else { //background job
            JobsList::JobEntry* job = new JobsList::JobEntry(pid, with_ampersand, time(nullptr), false);
            smash.jobs_list->addJob(pid, job, false);
        }
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
        free(og_cmd_line);
        _parse_delete(args, args_num);
        free(args);
    }
    if(pid == 0) { //child process
        setpgrp();
        if(containsWildcard(new_cmd_line)) {
            string bash = "bash";
            string flag = "-c";
            char* const argv[] = {(char* const)bash.c_str(), (char* const)flag.c_str(), new_cmd_line, nullptr};
            if(execv("/bin/bash", argv) < 0) {
                perror("smash error: execv failed");
                free(og_cmd_line);
                _parse_delete(args, args_num);
                free(args);
                exit(0);
            }
        }
        else {
            if(execvp(args[0], args) < 0) {
                perror("smash error: execvp failed");
                free(og_cmd_line);
                _parse_delete(args, args_num);
                free(args);
                exit(0);
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
                free(og_cmd_line);
                _parse_delete(args, args_num);
                free(args);
                return;
            }
            string empty = "";
            smash.curr_cmd_line = (char*)empty.c_str();
            //free(og_cmd_line);
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
    if(args == nullptr) {
        cerr << "smash error:> \"" << cmd_line << "\"" << endl;
        free(args); 
    }
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
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(close(1) < 0) {
        perror("smash error: close failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(first_index == last_index) { // "cmd1 > cmd2" - write over
        fd = open((char*)second_cmd.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
        if(fd < 0) {
            perror("smash error: open failed");
            dup2(out,1);
            _parse_delete(args, args_num);
            free(args);
            return;
        }

    }
    else { // "cmd1 >> cmd2" - append
        fd = open((char*)second_cmd.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);
        if(fd < 0) {
            perror("smash error: open failed");
            dup2(out,1);
            _parse_delete(args, args_num);
            free(args);
            return;
        }
    }
    string command = firstWord(first_cmd.c_str());
    if (command == "chprompt") {
        ChpromptCommand chprompt(first_cmd.c_str());
        chprompt.execute();
    } else if (command == "showpid" || command == "showpid&") {
        ShowPidCommand showpid(first_cmd.c_str());
        showpid.execute();
    } else if (command == "pwd" || command == "pwd&") {
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
    else if(command == "jobs" || command == "jobs&") {
        JobsCommand jobs(first_cmd.c_str());
        jobs.execute();
    }
    else if(command == "fg") {
        ForegroundCommand fg(first_cmd.c_str());
        fg.execute();
    }
    else if(command == "setcore") {
        SetcoreCommand setcore(first_cmd.c_str());
        setcore.execute();
    }
    else if(command == "bg") {
        BackgroundCommand bg(first_cmd.c_str());
        bg.execute();
    }
    else if(command == "kill") {
        KillCommand kill_cmd(first_cmd.c_str());
        kill_cmd.execute();
    }
    else if(command == "getfileinfo") {
        GetFileTypeCommand getfileinfo(first_cmd.c_str());
        getfileinfo.execute();
    }
    else if(command == "chmod") {
        ChmodCommand chmod(first_cmd.c_str());
        chmod.execute();
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

PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line) {}

void PipeCommand::execute() {
    char** args = (char**)malloc(sizeof(char*)* COMMAND_MAX_ARGS);
    char new_cmd_line[COMMAND_ARGS_MAX_LENGTH]; //cmd_line without '&'
    int args_num = _parseCommandLine(new_cmd_line, args);
    strcpy(new_cmd_line, cmd_line);
    string cmd_str = new_cmd_line;
    string first_cmd;
    string second_cmd;
    bool err;
    unsigned int index = string(new_cmd_line).find_first_of("|");
    first_cmd = string(new_cmd_line).substr(0, index);
    if (string(new_cmd_line).find_first_of("&") != string::npos && string(new_cmd_line).find_first_of("&") == index + 1) { // "|&"
		second_cmd = string(new_cmd_line).substr(index + 2);
		err = true;
	}
	else // "|"
	{
		second_cmd = string(new_cmd_line).substr(index + 1);
	}
    first_cmd = _trim(first_cmd);
    second_cmd = _trim(second_cmd);
    int my_pipe[2]; //my_pipe[0] - read; my_pipe[1] - write
    if(pipe(my_pipe) < 0) {
        perror("smash error: pipe failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    pid_t pid1 = fork();
    if(pid1 < 0) {
        perror("smash error: fork failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(pid1 == 0) { //first child - writing
        setpgrp();
        if(!err) { //"|"
            if(dup2(my_pipe[1], 1) < 0) {
                perror("smash error: dup2 failed");
                _parse_delete(args, args_num);
                free(args);
                exit(0);
            }
        }
        else { // "|&"
            if(dup2(my_pipe[1], 2) < 0) {
                perror("smash error: dup2 failed");
                _parse_delete(args, args_num);
                free(args);
                exit(0);
            }
        }
        if(close(my_pipe[0] < 0)) {
            perror("smash error: close failed");
            _parse_delete(args, args_num);
            free(args);
            exit(0);
        }
        string command = firstWord(first_cmd.c_str());
        if (command == "chprompt") {
            ChpromptCommand chprompt(first_cmd.c_str());
            chprompt.execute();
        } else if (command == "showpid" || command == "showpid&") {
            ShowPidCommand showpid(first_cmd.c_str());
            showpid.execute();
        } else if (command == "pwd" || command == "pwd&") {
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
        else if(command == "jobs" || command == "jobs&") {
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
        else if(command == "setcore") {
            SetcoreCommand setcore(first_cmd.c_str());
            setcore.execute();
        }
        else if(command == "getfileinfo") {
            GetFileTypeCommand getfileinfo(first_cmd.c_str());
            getfileinfo.execute();
        }
        else if(command == "chmod") {
            ChmodCommand chmod(first_cmd.c_str());
            chmod.execute();
        }
        else {
            char place[10] = "bash";
            char flag[4] = "-c";
            char* const argv[] = { place, flag, (char*)first_cmd.c_str(), nullptr };
            if (execv("/bin/bash", argv) < 0) {
                perror("smash error: execv failed");
                _parse_delete(args, args_num);
                free(args);
                exit(0);
            }
        }
        exit(0);
    }
    pid_t pid2 = fork();
    if(pid2 < 0) {
        perror("smash error: fork failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(pid2 == 0) { //second child - reading
        setpgrp();
        if(dup2(my_pipe[0], 0) < 0) {
            perror("smash error: dup2 failed");
            _parse_delete(args, args_num);
            free(args);
            exit(0);
        }
        if(close(my_pipe[0])) {
            perror("smash error: close failed");
            _parse_delete(args, args_num);
            free(args);
            exit(0);
        }
        if(close(my_pipe[1])) {
            perror("smash error: close failed");
            _parse_delete(args, args_num);
            free(args);
            exit(0);
        }
        string command = firstWord(second_cmd.c_str());
        if (command == "chprompt") {
            ChpromptCommand chprompt(second_cmd.c_str());
            chprompt.execute();
        } else if (command == "showpid" || command == "showpid&") {
            ShowPidCommand showpid(second_cmd.c_str());
            showpid.execute();
        } else if (command == "pwd" || command == "pwd&") {
            GetCurrDirCommand pwd(second_cmd.c_str());
            pwd.execute();
        } else if (command == "cd") {
            ChangeDirCommand cd(second_cmd.c_str());
            cd.execute();
        }
        else if(command == "quit") {
            QuitCommand quit(second_cmd.c_str());
            quit.execute();
        }
        else if(command == "jobs" || command == "jobs&") {
            JobsCommand jobs(second_cmd.c_str());
            jobs.execute();
        }
        else if(command == "fg") {
            ForegroundCommand fg(second_cmd.c_str());
            fg.execute();
        }
        else if(command == "bg") {
            BackgroundCommand bg(second_cmd.c_str());
            bg.execute();
        }
        else if(command == "kill") {
            KillCommand kill_cmd(second_cmd.c_str());
            kill_cmd.execute();
        }
        else if(command == "setcore") {
            SetcoreCommand setcore(second_cmd.c_str());
            setcore.execute();
        }
        else if(command == "getfileinfo") {
            GetFileTypeCommand getfileinfo(second_cmd.c_str());
            getfileinfo.execute();
        }
        else if(command == "chmod") {
            ChmodCommand chmod(second_cmd.c_str());
            chmod.execute();
        }
        else {
            char place[10] = "bash";
            char flag[4] = "-c";
            char* const argv[] = { place, flag, (char*)second_cmd.c_str(), nullptr };
            if (execv("/bin/bash", argv) < 0) {
                perror("smash error: execv failed");
                _parse_delete(args, args_num);
                free(args);
                exit(0);
            }
        }
        exit(0);
    }
    // parent
    if(close(my_pipe[0]) < 0) {
        perror("smash error: close failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(close(my_pipe[1]) < 0) {
        perror("smash error: close failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(waitpid(pid1, nullptr, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
        _parse_delete(args, args_num);
        free(args);
        return;
    }
    if(waitpid(pid2, nullptr, WUNTRACED) < 0) {
        perror("smash error: waitpid failed");
        _parse_delete(args, args_num);
        free(args);
        return;
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
        //free(entry.second->og_cmd_line);
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
    cout << "smash: sending SIGKILL signal to " << this->jobs_map.size() << " jobs:" << endl;
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
        else if(result == -1 && errno == ECHILD){
            if(entry.second->og_cmd_line != nullptr) {
                free(entry.second->og_cmd_line);
            }
            delete entry.second;
            finished_jobs.push_back(entry.first);
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

AlarmList::AlarmList(): alarm_list() {}

AlarmList::~AlarmList() {
    for(auto it = this->alarm_list.begin(); it != this->alarm_list.end(); ++it) {
        delete *it;
    }
}

void AlarmList::addAlarm(string cmd_line, time_t timestamp, int duration, pid_t pid) {
    this->alarm_list.push_back(new AlarmEntry(cmd_line, timestamp, duration, pid));
}

void AlarmList::removeFinishedAlarms(time_t time) {
    if(this->alarm_list.empty()) {
        return;
    }
    for (auto it = this->alarm_list.begin(); it != this->alarm_list.end();) {
        int curr_diff = (*it)->duration - difftime(time, (*it)->timestamp);
        if (curr_diff < 0) {
            delete *it;
            it = this->alarm_list.erase(it);
        } else {
            ++it;
        }
    }
    return;
}

int AlarmList::lowestTime(time_t time) {
	if (this->alarm_list.empty()) {
		return -1;
    }
	int lowest_time = this->alarm_list[0]->duration - difftime(time, this->alarm_list[0]->timestamp);
	for (AlarmEntry* entry : this->alarm_list) {
		int current_difference = entry->duration - difftime(time, entry->timestamp);
		if (current_difference < lowest_time && current_difference > 0)
		{
			lowest_time = current_difference;
		}
	}
	return lowest_time;
}

void AlarmList::removeAlarmByPid(pid_t pid) {
    if(this->alarm_list.empty()) {
        return;
    }
    //AlarmEntry* toDelete;
    for(auto it = this->alarm_list.begin(); it != this->alarm_list.end(); ++it) {
        if((*it)->pid == pid) {
            //toDelete = (*it);
            this->alarm_list.erase(it);
        }
        break;
    }
    //delete toDelete;
}

AlarmList::AlarmEntry* AlarmList::getCurrAlarm(time_t time) {
    if(this->alarm_list.empty()) {
        return nullptr;
    }
    AlarmEntry* alarm;
    for(auto it = this->alarm_list.begin(); it != this->alarm_list.end(); ++it) {
        int curr_diff = (*it)->duration - difftime(time, (*it)->timestamp);
        if(curr_diff <= 0) {
            alarm = (*it);
            this->alarm_list.erase(it);
            return alarm;
        }
    }
    return nullptr;
}

SmallShell::SmallShell() {}

SmallShell::~SmallShell() {
    delete SmallShell::jobs_list;
    delete SmallShell::alarm_list;
}

void SmallShell::executeCommand(const char* cmd_line) {
    string command = firstWord(cmd_line);
    if(_isRedirection(cmd_line)) {
        RedirectionCommand redirect(cmd_line);
        redirect.execute();
        return;
    }
    if(_isPipe(cmd_line)) {
        PipeCommand pipe(cmd_line);
        pipe.execute();
        return;
    }
    else if(command == "") {
        return;
    }
    else if (command == "chprompt") {
        ChpromptCommand chprompt(cmd_line);
        chprompt.execute();
    } else if (command == "showpid" || command == "showpid&") {
        ShowPidCommand showpid(cmd_line);
        showpid.execute();
    } else if (command == "pwd" || command == "pwd&") {
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
    else if(command == "jobs" || command == "jobs&") {
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
    else if(command == "setcore") {
        SetcoreCommand setcore(cmd_line);
        setcore.execute();
    }
    else if(command == "kill") {
        KillCommand kill_cmd(cmd_line);
        kill_cmd.execute();
    }
    else if(command == "getfileinfo") {
        GetFileTypeCommand getfileinfo(cmd_line);
        getfileinfo.execute();
    }
    else if(command == "chmod") {
        ChmodCommand chmod(cmd_line);
        chmod.execute();
    }
    else if(command == "timeout") {
        TimeoutCommand timeout(cmd_line);
        timeout.execute();
    }
    else {
        ExternalCommand external(cmd_line);
        external.execute();
    }
}