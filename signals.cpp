#include "signals.h"
#include <signal.h>
#include <iostream>
#include "Commands.h"
#include <sys/signal.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z" << endl;
    if(smash.curr_pid != -1) {
        JobsList::JobEntry* job = new JobsList::JobEntry(smash.curr_pid, smash.curr_cmd_line, time(nullptr), true);
        smash.jobs_list->addJob(smash.curr_pid, job);
        if(kill(smash.curr_pid, SIGSTOP) < 0) {
            perror("smash error: kill failed");
        }
        cout << "smash: process " << smash.curr_pid << " was stopped" << endl;
        smash.curr_pid = -1;
    }
    smash.jobs_list->removeFinishedJobs();
}

void ctrlCHandler(int sig_num) {
    pid_t pid = SmallShell::curr_pid;
    cout << "smash: got ctrl-C" << endl;
    if(pid != -1) {
        cout << "smash: process " << pid << " was killed" << endl;
        if(kill(pid, 9) < 0) {
            perror("smash error: kill failed");
        }
    }
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}
