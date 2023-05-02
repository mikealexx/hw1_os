#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <iostream>

#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    // TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    while (true) {
        std::cout << SmallShell::prompt << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
        if(cmd_line == "z") {
            ctrlZHandler(23);
        }
    } 
    return 0;
}