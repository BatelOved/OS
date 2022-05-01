#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>


using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

string _ltrim(const std::string& s) {
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s) {
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s) {
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

void _freeArguments(char** args) {
  for (int i = 0; args[i]; i++) {
    free(args[i]);
  }
}

bool _isPipeCommand(const char* cmd_line) {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(cmd_line, args);

  bool flag = false;

  for (int i = 0; i < args_num; i++) {
    if (!strcmp(args[i], "|") || !strcmp(args[i], "|&")) {
      flag = true;
    }
  }

  _freeArguments(args);
  return flag;
}

bool _isIOCommand(const char* cmd_line) {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(cmd_line, args);

  bool flag = false;

  for (int i = 0; i < args_num; i++) {
    if (!strcmp(args[i], ">") || !strcmp(args[i], "<") || !strcmp(args[i], ">>") || !strcmp(args[i], "<<")) {
      flag = true;
    }
  }

  _freeArguments(args);
  return flag;
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

/***************************************** SmallShell methods *****************************************/

SmallShell::SmallShell(): prompt(new char[COMMAND_ARGS_MAX_LENGTH]), prev_dir(new char*[1]) {
  strcpy(this->prompt, "smash");
  *(this->prev_dir) = nullptr;
}

SmallShell::~SmallShell() {
  if (this->prev_dir) {
    delete[] *(this->prev_dir);
  }
  delete this->prev_dir;
  delete[] this->prompt;
}

char* SmallShell::getPrompt() const {
    return this->prompt;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (_isPipeCommand(cmd_line)) {
    return new PipeCommand(cmd_line);
  }
  // else if (_isIOCommand(cmd_line)) {
  //   return new RedirectionCommand(cmd_line);
  // }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
    return new ChangePrompt(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("bg") == 0) {
    return new BackgroundCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line);
  }
  else if (firstWord.compare("tail") == 0) {
    return new TailCommand(cmd_line);
  }
  else if (firstWord.compare("touch") == 0) {
    return new TouchCommand(cmd_line);
  }
  else if (firstWord.compare("timeout") == 0) {
    // TODO
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  //this->jobs.removeFinishedJobs();
  if (_isBackgroundComamnd(cmd_line) == false) {
    delete cmd;
  }
}

JobsList& SmallShell::getJobsList() {
  return this->jobs;
}

char** SmallShell::getLastPwd() {
  return this->prev_dir;
}

/***************************************** Command methods *****************************************/

Command::Command(const char* cmd_line) {
  this->cmd_line = new char[strlen(cmd_line)+1];
  strcpy(this->cmd_line, cmd_line);
}

Command::~Command() {
  delete[] cmd_line;
}


// Should add cases of failed waitpid and so on


/***************************************** Built-in commands *****************************************/

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line) {}

//chprompt
ChangePrompt::ChangePrompt(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ChangePrompt::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();

  if (args[1]) {
    strcpy(smash.getPrompt(), args[1]);
  }
    
  else {
    strcpy(smash.getPrompt(), "smash");
  }

  _freeArguments(args);
}

//showpid
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
  cout << "smash pid is " << getpid() << endl;
}

//pwd
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
  char buff[COMMAND_ARGS_MAX_LENGTH];
  cout << getcwd(buff, COMMAND_ARGS_MAX_LENGTH) << endl;
}

//cd
ChangeDirCommand::ChangeDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();

  char** lastPwd = smash.getLastPwd();
  char* path = args[1];

  char curr[COMMAND_ARGS_MAX_LENGTH];
  getcwd(curr, COMMAND_ARGS_MAX_LENGTH);

  if (args_num > 2) {
    cout<<"smash error: cd: too many arguments"<<endl;
  }

  else if (path && strcmp(path, "-") == 0) {
    if (!(*lastPwd)) {
      cout<<"smash error: cd: OLDPWD not set"<<endl;
    }
    else {
      if (chdir(*lastPwd) < 0) {
        perror("smash error: cd");
      }
      else {
        strcpy(*lastPwd, curr);
      }
    }
  }

  else {
    if (path && chdir(path) < 0) {
      perror("smash error: cd");
    }
    else {
      if (!(*lastPwd)) {
        *lastPwd = new char[COMMAND_ARGS_MAX_LENGTH];
      }
      strcpy(*lastPwd, curr);
    }
  }

  _freeArguments(args);
}

//jobs
JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  SmallShell& smash = SmallShell::getInstance();
  this->list = &smash.getJobsList();
}

void JobsCommand::execute() {
  list->printJobsList();
}

//kill
KillCommand::KillCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();
  JobsList jobs = smash.getJobsList();

  char* first_param = args[1];
  char* second_param = args[2];

  if(!first_param || !second_param) {
    cerr<<"smash error: kill: invalid arguments"<<endl;
    _freeArguments(args);
    return;
  } 
  else if(first_param[0] != '-') {
    cerr<<"smash error: kill: invalid arguments"<<endl;
    _freeArguments(args);
    return;
  }//maybe we should check the number of aguments in createcommand function

  unsigned int i;
  for(i=1; i<strlen(first_param); i++) {
    if(first_param[i]<'0' || first_param[i]>'9') {
      cerr<<"smash error: kill: invalid arguments"<<endl;
      _freeArguments(args);
      return;
    }
  }

  for(i=0; i<strlen(second_param); i++) {
    if(second_param[i]<'0' || second_param[i]>'9') {
      cerr<<"smash error: kill: invalid arguments"<<endl;
      _freeArguments(args);
      return;
    }
  }
  //Should be splitted to the execute method
  int signal=stoi(string(first_param+1));//Should be checked for correction
  int job_id=stoi(string(second_param));

  JobsList::JobEntry* to_kill=jobs.getJobById(job_id);

  if(!to_kill){
		cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
    _freeArguments(args);
		return;
	}

  pid_t pid_to_kill=jobs.getJobById(job_id)->getProcessID();
  if(kill(pid_to_kill, signal) == -1) {
    perror("smash error: kill failed"); //The error handling should be checked
    _freeArguments(args);
    return;
  }
  else {
    cout<<"signal number "<<signal<<" was sent to pid "<<pid_to_kill<<endl;
  }

  if(signal == SIGKILL) {
    smash.getJobsList().removeJobById(to_kill->getJobID());
    delete to_kill->getCommand();
  }
  if(signal == SIGSTOP) {
    to_kill->stopTheJob();
  }
  if(signal == SIGCONT) {
    to_kill->turnToBG();
  }
  _freeArguments(args);
}

//fg
ForegroundCommand::ForegroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void ForegroundCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int number_of_arguments = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();
  JobsList jobs = smash.getJobsList();

  if(number_of_arguments > 2) {
    cerr<<"smash error: fg: invalid arguments";
    _freeArguments(args);
    return;
  }

  JobsList::JobEntry* curr_job;
  if(number_of_arguments == 1) {
    JobsList::JobEntry* last = jobs.getLastJob(nullptr);
    if(!last) {
      cerr<<"smash error: fg: jobs list is empty"<<endl;
      _freeArguments(args);
      return;
    }
    curr_job = last;
  }
  else { //The number of arguments is 2
    //Being used in some places, maybe it would be better in a seperate function
    unsigned int i;
    for(i=0; i<strlen(args[1]); i++) {
      if(args[1][i]<'0' || args[1][i]>'9') {
        cerr<<"smash error: fg: invalid arguments"<<endl;
        _freeArguments(args);
        return;
      }
    }

    int job_id = stoi(string(args[1]));
    JobsList::JobEntry* job_to_fg = jobs.getJobById(job_id);
    if(!job_to_fg) {
      cerr<<"smash error: fg: job-id "<<job_id<<" does not exist"<<endl;
      _freeArguments(args);
      return;
    }

    curr_job = job_to_fg;
  }

  cout<<curr_job->getCommand()->getCmdLine()<<" : "<<curr_job->getProcessID()<<endl;
  if(curr_job->isStopped()) {
    if(kill(curr_job->getProcessID(), SIGCONT) == -1) {
      perror("smash error: kill failed");
      _freeArguments(args);
      return;
    }
  }

  JobsList::JobEntry copy = *curr_job;
  jobs.removeJobById(copy.getJobID());
  copy.turnToFG();

  int status;
  if(waitpid(copy.getProcessID(), &status, WUNTRACED) != copy.getProcessID()) {
    perror("smash error: waitpid failed");
  }

  _freeArguments(args);
}

//bg
BackgroundCommand::BackgroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void BackgroundCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int number_of_arguments = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();
  JobsList jobs = smash.getJobsList();

  if(number_of_arguments > 2) {
    cerr<<"smash error: bg: invalid arguments";
    _freeArguments(args);
    return;
  }

  JobsList::JobEntry* curr_job;
  if(number_of_arguments == 1) {
    JobsList::JobEntry* last = jobs.getLastStoppedJob();
    if(!last) {
      cerr<<"smash error: bg: there is no stopped jobs to resume"<<endl;
      _freeArguments(args);
      return;
    }
    curr_job = last;
  }
  else { //The number of arguments is 2
    //Being used in some places, maybe it would be better in a seperate function
    unsigned int i;
    for(i=0; i<strlen(args[1]); i++) {
      if(args[1][i]<'0' || args[1][i]>'9') {
        cerr<<"smash error: bg: invalid arguments"<<endl;
        _freeArguments(args);
        return;
      }
    }

    int job_id = stoi(string(args[1]));
    JobsList::JobEntry* job_to_bg = jobs.getJobById(job_id);
    if(!job_to_bg) {
      cerr<<"smash error: bg: job-id "<<job_id<<" does not exist"<<endl;
      _freeArguments(args);
      return;
    }
    if(!job_to_bg->isStopped()) {
      cerr<<"smash error: bg: job-id "<<job_id<<" is already running in the background"<<endl;
      _freeArguments(args);
      return;
    }

    curr_job = job_to_bg;
  }

  cout<<curr_job->getCommand()->getCmdLine()<<" : "<<curr_job->getProcessID()<<endl;
  if(kill(curr_job->getProcessID(), SIGCONT) == -1) {
    perror("smash error: kill failed");
    _freeArguments(args);
    return;
  }

  curr_job->turnToBG();
  
  _freeArguments(args);
}

//quit
QuitCommand::QuitCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  smash.getJobsList().killAllJobs();
  kill(getpid(),SIGKILL);
}

/***************************************** Externals commands *****************************************/

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  pid_t p = fork();

	if (p == 0) {
    const char* args[] = {"/bin/bash", "-c", this->getCmdLine(), NULL};
    execv(args[0], (char**)args);
	}
  else {
    if (_isBackgroundComamnd(this->getCmdLine())) {
      smash.getJobsList().addJob(this, p);
    }
    else {
      wait(NULL);
    }
  }
}

/***************************************** Special commands *****************************************/

//pipe
PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line), left_cmd(new char[COMMAND_ARGS_MAX_LENGTH]),
                                                                        right_cmd(new char[COMMAND_ARGS_MAX_LENGTH]) {
  const string str(cmd_line);
  _trim(str);

  size_t left_cmd_end = str.find_first_of("|");
  strcpy(left_cmd, str.substr(0, left_cmd_end).c_str());

  const string sub_str(str.substr(left_cmd_end+1));
  _trim(sub_str);

  strcpy(right_cmd, sub_str.c_str());
}

PipeCommand::~PipeCommand() {
  delete[] left_cmd;
  delete[] right_cmd;
}

void PipeCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  int fd[2];
  pipe(fd);

  if (fork() == 0) {
    // first child 
    dup2(fd[1],1);
    close(fd[0]);
    close(fd[1]);
    smash.executeCommand(this->left_cmd);
    setpgrp();
    //execv(“/bin/ls”,…);
  }
  if (fork() == 0) { 
    // second child 
    dup2(fd[0],0);
    close(fd[0]);
    close(fd[1]);
    smash.executeCommand(this->right_cmd);
    setpgrp();
    //execv(“/bin/more”,…);
  }
  close(fd[0]);
  close(fd[1]);

  // wait ??

}

//IO
RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line) {

}

void RedirectionCommand::execute() {

}

//tail
TailCommand::TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void TailCommand::execute() {

}

//touch
TouchCommand::TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void TouchCommand::execute() {

}

/***************************************** JobsList methods *****************************************/

JobsList::JobsList(): jobs_vector(vector<JobEntry>()), max_id(0) {}

JobsList::~JobsList() {}

void JobsList::addJob(Command* cmd, pid_t child_pid, bool isStopped) {
    jobs_vector.push_back(JobEntry(this->max_id++, child_pid, isStopped, cmd));
}

void JobsList::printJobsList() {
  for (JobEntry& iter : jobs_vector) {
    cout << "[" << iter.getJobID() << "] " << iter.getCommand()->getCmdLine() << " : " << iter.getProcessID() << " " << iter.returnDiffTime() << " secs";
    if (iter.isStopped()) {
      cout << " (stopped)";
    }
    cout << endl;
  }
}

void JobsList::killAllJobs() {
  cout<<"smash: killing all jobs"<<endl;

  for(JobEntry& iter : jobs_vector) {
    if(kill(iter.getProcessID(), SIGKILL) == -1) {
      cerr<<"error: kill signal failed";
    }
    else {
      int status;
      if(waitpid(iter.getProcessID(), &status, 0) != iter.getProcessID()) {
        cerr<<"error: waitpid failed";
      }
    }
  }
  jobs_vector.clear();
  max_id=0;
}

void JobsList::removeFinishedJobs() {
  int status;
  for(JobEntry& iter : jobs_vector) {
    pid_t is_finished = waitpid(iter.getProcessID(), &status, WNOHANG);
    if (WIFEXITED(status)) {
      cout << is_finished << endl;
      //killAllJobs();
      //delete iter->getCommand();
      //jobs_vector.erase(iter);
    }
  }

  if(jobs_vector.empty()) {
    max_id=0;
  }
  else {
    max_id=jobs_vector.back().getJobID();
  }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
  for(JobEntry& iter : jobs_vector) {
    if(iter.getJobID() == jobId) {
      return &iter;
    }
  }

  return nullptr;
}

void JobsList::removeJobById(int jobId) {
  for(JobEntry& iter : jobs_vector) {
    if(iter.getJobID() == jobId) {
      jobs_vector.erase(std::vector<JobEntry>::iterator(&iter));
    }
  }
  if (jobs_vector.empty()) {
    max_id = 0;
  }
  else {
    max_id = jobs_vector.back().getJobID();
  }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
  if(jobs_vector.empty()) {
    return nullptr;
  }
  
  if(lastJobId) {
    *lastJobId = jobs_vector.back().getJobID();
  }

  return &jobs_vector.back();

}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId) {
  JobEntry* last_stopped_job = nullptr;
  for(JobEntry& iter : jobs_vector) {
    if(iter.isStopped()) {
      last_stopped_job = &iter;
    }
  }

  if(last_stopped_job && jobId) {
    *jobId = last_stopped_job->getJobID();
  }

  return last_stopped_job;
}
