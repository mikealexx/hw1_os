#include "signals.h"
#include <signal.h>
#include <iostream>
#include "Commands.h"
#include <sys/signal.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    // TODO: Add your implementation
    cout << getpid() << endl;

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
