#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  cout << "smash: got ctrl-Z" << endl;

  SmallShell& smash = SmallShell::getInstance();

  if(smash.getCurrentPid() == 0) {
    return;
  }
  
  if(!smash.getJobsList().jobExistsByPID(smash.getCurrentPid())) {
    smash.getJobsList().addJob(smash.getCurrCmd(), smash.getCurrentPid(), true);
  }
  else {
    smash.getJobsList().jobExistsByPID(smash.getCurrentPid())->stopProcess();
  }

  int res = kill(smash.getCurrentPid(), SIGSTOP);
  if(res == -1) {
    perror("smash error: kill failed");
  }

  cout << "smash: process "<<smash.getCurrentPid()<<" was stopped"<< std::endl;
/*pid_t p = fork();

	if (p == 0) {

    setpgrp();
	}
  else {
 
    if(!smash.getJobsList().jobExistsByPID(p)) {
      smash.getJobsList().addJob(smash.getCurrCmd(), smash.getCurrentPid(), true);
    }
    cout<<"Before kill";
    kill(smash.getCurrentPid(), SIGSTOP);
    cout<<"after kill";
    waitpid(p, nullptr,WNOHANG);
  }*/
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;

  SmallShell& smash = SmallShell::getInstance();

  if(smash.getCurrentPid() == 0) {
    return;
  }

  //if(smash.getJobsList().jobExistsByPID(smash.getCurrentPid())) {
  //  smash.getJobsList().removeJobById(smash.getJobsList().jobExistsByPID(smash.getCurrentPid())->getJobID());
  //}

  int res = kill(smash.getCurrentPid(), SIGKILL);
  if(res == -1) {
    perror("smash error: kill failed");
  }

  JobsList::JobEntry* job_to_kill = smash.getJobsList().jobExistsByPID(smash.getCurrentPid());
  if(job_to_kill) {
    //There is a problem. Probably with the case we take a bg command to the fg and kill
    smash.getJobsList().removeJobById(job_to_kill->getJobID());
  }

  cout << "smash: process "<<smash.getCurrentPid()<<" was killed"<<std::endl;
/*  pid_t p = fork();

	if (p == 0) {
    smash.executeCommand((string("kill -") + to_string(SIGKILL) + string(" ") + to_string(getppid())).c_str());
    setpgrp();
	}
  else {
    wait(NULL);
  }*/

}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation

  cout<<"smash: got an alarm" << endl;

  SmallShell& smash = SmallShell::getInstance();
  
  cout << "smash: " << smash.getCurrCmd() << " timed out!" << endl;
  /*
  search which command caused the alarm, send a SIGKILL to its process and print:
smash: [command-line] timed out!
  */
}

