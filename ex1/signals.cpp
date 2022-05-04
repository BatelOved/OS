#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {

  SmallShell& smash = SmallShell::getInstance();

  if(smash.getCurrentPid() == 0) {
    return;
  }
  cout << "got ctrl-Z" << endl;
  
  pid_t p = fork();

	if (p == 0) {

    setpgrp();
	}
  else {
    if(!smash.getJobsList().jobExistsByPID(p)) {
      smash.getJobsList().addJob(smash.getCurrCmd(), smash.getCurrentPid(), true);
    }
    
    kill(smash.getCurrentPid(), SIGSTOP);
    
    //waitpid(p, nullptr,WNOHANG);
  }
}

void ctrlCHandler(int sig_num) {
  cout << "got ctrl-C" << endl;

  SmallShell& smash = SmallShell::getInstance();

  pid_t p = fork();

	if (p == 0) {
    smash.executeCommand((string("kill -") + to_string(SIGKILL) + string(" ") + to_string(getppid())).c_str());
    setpgrp();
	}
  else {
    wait(NULL);
  }

}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

