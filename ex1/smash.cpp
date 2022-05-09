#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"


#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    struct sigaction sa = {0};
    sa.sa_handler=&alarmHandler;
    sa.sa_flags=SA_RESTART;
    if(sigaction(SIGALRM, &sa, nullptr)==-1) {
    //if(signal(SIGALRM , alarmHandler)==SIG_ERR) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();

    while(true) {
        std::cout << smash.getPrompt() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}
