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
        if(smash.jobs_list->getJobByPid(smash.curr_pid) == nullptr) { //job is not already in jobs list
            JobsList::JobEntry* job = new JobsList::JobEntry(smash.curr_pid, smash.curr_cmd_line, time(nullptr), true);
            smash.jobs_list->addJob(smash.curr_pid, job);
        }
        else {
            smash.jobs_list->getJobByPid(smash.curr_pid)->start_time = time(nullptr);
            smash.jobs_list->getJobByPid(smash.curr_pid)->stopped = true;
        }
        if(kill(smash.curr_pid, SIGSTOP) < 0) {
            perror("smash error: kill failed");
        }
        cout << "smash: process " << smash.curr_pid << " was stopped" << endl;
        smash.curr_pid = -1;
    }
    smash.jobs_list->removeFinishedJobs();
}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    pid_t pid = SmallShell::curr_pid;
    cout << "smash: got ctrl-C" << endl;
    if(pid != -1) {
        cout << "smash: process " << pid << " was killed" << endl;
        int result = kill(pid, SIGINT);
        if(result < 0) {
            perror("smash error: kill failed");
        }
    }
    JobsList::JobEntry* job = smash.jobs_list->getJobByPid(pid);
    if(job != nullptr) {
        smash.jobs_list->removeJobById(smash.jobs_list->getJobIdByPid(pid));
    }
    smash.jobs_list->removeFinishedJobs();
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
}
