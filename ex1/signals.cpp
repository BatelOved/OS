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
  
  JobsList::JobEntry* job_in_list = smash.getJobsList().jobExistsByPID(smash.getCurrentPid());
  if(!job_in_list) {
    smash.getJobsList().addJob(smash.getCurrCmd(), smash.getCurrentPid(), true);
  }
  else {
    job_in_list->stopProcess();
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
  // maybe we should work on a specific time at the first of the handler instead of checking
  // every time and maybe creating a difference in the current time between jobs

  cout<<"smash: got an alarm" << endl;

  SmallShell& smash = SmallShell::getInstance();

  smash.getJobsList().removeFinishedJobs();
  
  TimeoutObject* curr = smash.getJobsList().getCurrentTimeout();
  if(!curr){
    smash.getJobsList().continueNextAlarm();
    return;
  }

  
  if(!smash.getJobsList().jobExistsByPID(curr->getPID())) {
    smash.getJobsList().removeTimeoutObject(curr->getPID());

    smash.getJobsList().continueNextAlarm();
    return;
  }
  
  if(kill(curr->getPID(), SIGKILL) == -1) {
    perror("smash error: kill failed");
  }
  else {
    cout << "smash: " << curr->getCommand()->getCmdLine() << " timed out!" << endl;
  }

  //Maybe it's already covered by the case of remove_finished_jobs
  JobsList::JobEntry* to_delete = smash.getJobsList().jobExistsByPID(curr->getPID());
  smash.getJobsList().removeJobById(to_delete->getJobID());
  smash.getJobsList().removeTimeoutObject(curr->getPID());

  smash.getJobsList().continueNextAlarm();

//Have to delete the object

  /*
  search which command caused the alarm, send a SIGKILL to its process and print:
smash: [command-line] timed out!
  */
}

