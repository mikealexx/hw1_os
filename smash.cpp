#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <iostream>

#include "Commands.h"
#include "signals.h"

int nop() {
    return 5;
}

int main(int argc, char* argv[]) {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction sa;
    sa.sa_handler = alarmHandler;
    sa.sa_flags = SA_RESTART; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("smash error: failed to set ctrl-C handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    int i=1;
    while (true) {
        //if(i==40) {
        //    int j = nop();
        //}
        std::cout << SmallShell::prompt << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
        i++;
    } 
    return 0;
}