#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	cout << "got ctrl-Z" << endl;

  SmallShell& smash = SmallShell::getInstance();

  pid_t p = fork();

	if (p == 0) {
    smash.getJobsList().printJobsList();
    smash.getJobsList().addJob(smash.CreateCommand("sleep"), getpid());
    int x = 5;
    //smash.executeCommand((string("kill -") + to_string(SIGSTOP) + string(" ") + to_string(getpid())).c_str());

    setpgrp();
	}
  else {
    wait(NULL);
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

