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
  if(smash.getCurrentPid() == 0) { // Ctrl Z on smash
    return;
  }
  
  JobsList::JobEntry* job_in_list = smash.getJobsList().jobExistsByPID(smash.getCurrentPid());
  if(!job_in_list) { // There is no job for the process
    smash.getJobsList().addJob(smash.getCurrCmd(), smash.getCurrentPid(), true);
  }
  else { //The job is already exist
    job_in_list->stopProcess();
  }

  if(kill(smash.getCurrentPid(), SIGSTOP) == -1) {
    perror("smash error: kill failed");
  }

  cout << "smash: process "<<smash.getCurrentPid()<<" was stopped"<< std::endl;

}

void ctrlCHandler(int sig_num) {

  cout << "smash: got ctrl-C" << endl;

  SmallShell& smash = SmallShell::getInstance();
  if(smash.getCurrentPid() == 0) {
    return;
  }

  if(kill(smash.getCurrentPid(), SIGKILL) == -1) {
    perror("smash error: kill failed");
  }

  JobsList::JobEntry* job_to_kill = smash.getJobsList().jobExistsByPID(smash.getCurrentPid());
  if(job_to_kill) { // If the process has a job, we need to delete it
    smash.getJobsList().removeJobById(job_to_kill->getJobID());
  }

  cout << "smash: process "<<smash.getCurrentPid()<<" was killed"<<std::endl;

}

void alarmHandler(int sig_num) {
  // maybe we should work on a specific time at the first of the handler instead of checking
  // every time and maybe creating a difference in the current time between jobs

  cout<<"smash: got an alarm" << endl;

  SmallShell& smash = SmallShell::getInstance();

  // The time we recieved the signal
  time_t current_time = time(nullptr);
  
  TimeoutObject* curr = smash.getJobsList().getCurrentTimeout(current_time);
  if(!curr){
    smash.getJobsList().continueNextAlarm();
    return;
  }
  
  // Checks if the process is still running
  if(waitpid(curr->getPID(),nullptr,WNOHANG) == 0) { 
    if(kill(curr->getPID(), SIGKILL) == -1) {
      perror("smash error: kill failed");
    }
    else {
      cout << "smash: " << curr->getCommand()->getCmdLine() << " timed out!" << endl;
    }
  }

  JobsList::JobEntry* to_delete = smash.getJobsList().jobExistsByPID(curr->getPID());
  if(to_delete) { // If a job of the process exists
    smash.getJobsList().removeJobById(to_delete->getJobID());
  }
  smash.getJobsList().removeTimeoutObject(curr->getPID());

  // Start the next alarm
  smash.getJobsList().continueNextAlarm();

}

