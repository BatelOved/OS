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

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
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

void freeArguments(char** args) {
  for(char* iter = args[0]; iter;) {
    char* to_delete = iter;
    ++iter;
    free(to_delete);
  }
}
// TODO: Add your implementation for classes in Commands.h 


/***************************************** SmallShell methods *****************************************/

SmallShell::SmallShell(): prompt((char*)malloc(COMMAND_ARGS_MAX_LENGTH)), prev_dir((char**)malloc(COMMAND_ARGS_MAX_LENGTH)) {
  strcpy(this->prompt, "smash");
  *(this->prev_dir) = nullptr;
}

SmallShell::~SmallShell() {
  free(this->prompt);
  free(this->prev_dir);
}

std::ostream& SmallShell::getPrompt(std::ostream& os) const {
    return os << this->prompt;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  char** args = (char**)malloc(COMMAND_MAX_ARGS+1);
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  _parseCommandLine(cmd_line, args);

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, this->prev_dir);
  }
  else if (firstWord.compare("chprompt") == 0) {
    return new ChangePrompt(cmd_line, args[1], this->prompt);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, &(this->jobs));
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, &(this->jobs));
  }
  else if (firstWord.compare("bg") == 0) {
    return new BackgroundCommand(cmd_line, &(this->jobs));
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line, &(this->jobs), args[1], args[2]);
  }
  else if (firstWord.compare("quit") == 0) {
    // TODO
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
    return new ExternalCommand(cmd_line, this);
  }
  free(args); //should write a different free functions. that's a memory leak
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {

  //Creating a new command
  Command* cmd = CreateCommand(cmd_line);

  //Cleaning the jobs list from inrelevant jobs
  this->jobs.removeFinishedJobs();

  //Executes the command(using a virtual method of the command)
  cmd->execute();

  //free(cmd);
}

void SmallShell::addJob(Command* cmd, bool isStopped) {
  jobs.addJob(cmd);
}



Command::Command(const char* cmd_line) {
  this->cmd_line = (char*)malloc(strlen(cmd_line)+1);
  strcpy(this->cmd_line, cmd_line);
}

Command::~Command() { }


// Should add cases of failed waitpid and so on


/***************************************** Built-in commands *****************************************/

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line) { }


//chprompt
ChangePrompt::ChangePrompt(const char* cmd_line, const char* prompt_name, char* prompt): BuiltInCommand(cmd_line) {
  if (prompt_name)
    strcpy(prompt, prompt_name);
  else
    strcpy(prompt, "smash");
}


//showpid
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) { }

void ShowPidCommand::execute() {
  cout << "smash pid is " << getpid() << endl;
}


//pwd
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) { }

void GetCurrDirCommand::execute() {
  char* buff = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  cout << getcwd(buff, COMMAND_ARGS_MAX_LENGTH) << endl;
  free(buff);
}


//cd
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), 
            curr_dir((char*)malloc(COMMAND_ARGS_MAX_LENGTH)), prev_dir((char*)malloc(COMMAND_ARGS_MAX_LENGTH)), 
                                                                      path((char*)malloc(COMMAND_ARGS_MAX_LENGTH)) {
  char** args = (char**)malloc(COMMAND_ARGS_MAX_LENGTH);
  string cmd_s = _trim(string(cmd_line));
  _parseCommandLine(cmd_line, args);

  if (args[2])
    perror("smash error: cd: too many arguments");

  if (!(*plastPwd)) {
    if (strcmp(args[1], "-") == 0) {
      perror("smash error: cd: OLDPWD not set");
    }
    else {
      *plastPwd = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
      getcwd(*plastPwd, COMMAND_ARGS_MAX_LENGTH);
    }
  }
  getcwd(this->curr_dir, COMMAND_ARGS_MAX_LENGTH);
  strcpy(this->prev_dir, *plastPwd);
  strcpy(*plastPwd, this->curr_dir);
  strcpy(this->path, args[1]);
}

void ChangeDirCommand::execute() {
  char* temp = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);

  if (strcmp(this->path, "-") == 0)
    strcpy(temp, this->prev_dir);
  else
    strcpy(temp, this->path);

  getcwd(this->prev_dir, COMMAND_ARGS_MAX_LENGTH);
  chdir(temp);
  getcwd(this->curr_dir, COMMAND_ARGS_MAX_LENGTH);

  free(temp);
}


//jobs
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), list(jobs) { }

void JobsCommand::execute() {
    list->printJobsList();
}


//kill
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs, const char* first_param, const char* second_param) : BuiltInCommand(cmd_line){ 
  if(!first_param || !second_param) {
    cerr<<"smash error: kill: invalid arguments"<<endl;
    return;
  } 
  else if(first_param[0] != '-') {
    cerr<<"smash error: kill: invalid arguments"<<endl;
    return;
  }//maybe we should check the number of aguments in createcommand function

  int i;
  for(i=1; i<strlen(first_param); i++) {
    if(first_param[i]<'0' || first_param[i]>'9') {
      cerr<<"smash error: kill: invalid arguments"<<endl;
      return;
    }
  }

  for(i=0; i<strlen(second_param); i++) {
    if(second_param[i]<'0' || second_param[i]>'9') {
      cerr<<"smash error: kill: invalid arguments"<<endl;
      return;
    }
  }
  //Should be splitted to the execute method
  int signal=stoi(string(first_param+1));//Should be checked for correction
  int job_id=stoi(string(second_param));

  JobsList::JobEntry* to_kill=jobs->getJobById(job_id);

  if(!to_kill){
		cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
		return;
	}

  pid_t pid_to_kill=to_kill->getProcessID();
  if(kill(pid_to_kill, signal) == -1) {
    perror("smash error: kill failed"); //The error handling should be checked
    return;
  }
  else {
    cout<<"signal number "<<signal<<" was sent to pid"<<pid_to_kill<<endl;
  }

  if(signal == SIGSTOP) {
    to_kill->stopTheJob();
  }
  if(signal == SIGCONT) {
    to_kill->turnToBG();
  }
}

void KillCommand::execute() {

}


//fg
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){ 
  char** args=(char**)malloc(COMMAND_MAX_ARGS);
  int number_of_arguments = _parseCommandLine(cmd_line, args);

  if(number_of_arguments > 2) {
    cerr<<"smash error: fg: invalid arguments";
    freeArguments(args);
    return;
  }

  JobsList::JobEntry* curr_job;
  if(number_of_arguments == 1) {
    JobsList::JobEntry* last = jobs->getLastJob(nullptr);
    if(!last) {
      cerr<<"smash error: fg: jobs list is empty"<<endl;
      freeArguments(args);
      return;
    }
    curr_job = last;
  }
  else { //The number of arguments is 2
    //Being used in some places, maybe it would be better in a seperate function
    int i;
    for(i=0; i<strlen(args[1]); i++) {
      if(args[1][i]<'0' || args[1][i]>'9') {
        cerr<<"smash error: fg: invalid arguments"<<endl;
        freeArguments(args);
        return;
      }
    }

    int job_id = stoi(string(args[1]));
    JobsList::JobEntry* job_to_fg = jobs->getJobById(job_id);
    if(!job_to_fg) {
      cerr<<"smash error: fg: job-id "<<job_id<<" does not exist"<<endl;
      freeArguments(args);
      return;
    }

    curr_job = job_to_fg;
  }

  cout<<curr_job->getCommand().getCmdLine()<<" : "<<curr_job->getProcessID()<<endl;
  if(curr_job->isStopped()) {
    if(kill(curr_job->getProcessID(), SIGCONT) == -1) {
      perror("smash error: kill failed");;;;;;;;;;;;;;;;;;
      return;
    }
  }

  JobsList::JobEntry copy = *curr_job;
  jobs->removeJobById(copy.getJobID());
  copy.turnToFG();

  int status;
  if(waitpid(copy.getProcessID(), &status, WUNTRACED) != copy.getProcessID()) {
    perror("smash error: waitpid failed");
  }

  freeArguments(args);
}

void ForegroundCommand::execute() {

}


//bg
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line){ 
  char** args=(char**)malloc(COMMAND_MAX_ARGS);
  int number_of_arguments = _parseCommandLine(cmd_line, args);

  if(number_of_arguments > 2) {
    cerr<<"smash error: bg: invalid arguments";
    freeArguments(args);
    return;
  }

  JobsList::JobEntry* curr_job;
  if(number_of_arguments == 1) {
    JobsList::JobEntry* last = jobs->getLastStoppedJob(nullptr);
    if(!last) {
      cerr<<"smash error: bg: there is no stopped jobs to resume"<<endl;
      freeArguments(args);
      return;
    }
    curr_job = last;
  }
  else { //The number of arguments is 2
    //Being used in some places, maybe it would be better in a seperate function
    int i;
    for(i=0; i<strlen(args[1]); i++) {
      if(args[1][i]<'0' || args[1][i]>'9') {
        cerr<<"smash error: bg: invalid arguments"<<endl;
        freeArguments(args);
        return;
      }
    }

    int job_id = stoi(string(args[1]));
    JobsList::JobEntry* job_to_bg = jobs->getJobById(job_id);
    if(!job_to_bg) {
      cerr<<"smash error: bg: job-id "<<job_id<<" does not exist"<<endl;
      freeArguments(args);
      return;
    }
    if(!job_to_bg->isStopped()) {
      cerr<<"smash error: bg: job-id "<<job_id<<" is already running in the background"<<endl;
      freeArguments(args);
      return;
    }

    curr_job = job_to_bg;
  }

  cout<<curr_job->getCommand().getCmdLine()<<" : "<<curr_job->getProcessID()<<endl;
  if(kill(curr_job->getProcessID(), SIGCONT) == -1) {
    perror("smash error: kill failed");;;;;;;;;;;;;;;;;;
    return;
  }

  curr_job->turnToBG();
  
  freeArguments(args);
}

void BackgroundCommand::execute() {

}


//quit - dont even try. your computer will crush
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) {
  jobs->killAllJobs();
  //kill(getpid(),9);
}

void QuitCommand::execute() { }


/***************************************** Externals commands *****************************************/

ExternalCommand::ExternalCommand(const char* cmd_line, SmallShell* p_smash): Command(cmd_line) {
  pid_t p = fork();

	if (p == 0) {
    const char* args[] = {"/bin/bash", "-c", cmd_line, NULL};
    execv(args[0], (char**)args);
	}
  else {
    wait(NULL);
    p_smash->addJob(this);
  }
}

void ExternalCommand::execute() { 

}


/***************************************** Special commands *****************************************/

//tail
TailCommand::TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line){

}

void TailCommand::execute() {

}


//touch
TouchCommand::TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line){

}

void TouchCommand::execute() {

}


/***************************************** JobsList methods *****************************************/

JobsList::JobsList() : jobs_vector(vector<JobEntry>()), max_id(0) {}

JobsList::~JobsList() {}
//there is probably a mess with the max_id. i will fix that
void JobsList::addJob(Command* cmd, bool isStopped) {
    jobs_vector.push_back(JobEntry(this->max_id++, getpid(), isStopped, cmd));
}

void JobsList::printJobsList() {
  for (JobEntry& iter : jobs_vector) {
    cout << "[" << iter.getJobID() << "]" << iter.getCommand().getCmdLine() << ":" << iter.getProcessID() << iter.returnDiffTime();
    if (iter.isStopped()) {
      cout << "(stopped)";
    }
    cout << endl;
  }
}

void JobsList::killAllJobs() {
  cout<<"smash: killing all jobs";

  for(JobEntry& iter:jobs_vector) {
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
  for(vector<JobEntry>::iterator iter = jobs_vector.begin(); iter != jobs_vector.end(); iter++) {
    int is_finished = waitpid((*iter).getProcessID(), nullptr, WNOHANG);

    if((*iter).getProcessID() == is_finished) {
      jobs_vector.erase(iter);
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
  for(JobEntry& iter:jobs_vector) {
    if(iter.getJobID() == jobId) {
      return &iter;
    }
  }

  return nullptr;
}

void JobsList::removeJobById(int jobId) {
  for(vector<JobEntry>::iterator iter = jobs_vector.begin(); iter != jobs_vector.end(); iter++) {
    if((*iter).getJobID() == jobId) {
      jobs_vector.erase(iter);

      if(jobs_vector.empty()) {
        max_id = 0;
      }
      else {
        max_id = jobs_vector.back().getJobID();
      }
    }
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
  for(JobEntry& iter:jobs_vector) {
    if(iter.isStopped()) {
      last_stopped_job = &iter;
    }
  }

  if(last_stopped_job && jobId) {
    *jobId = last_stopped_job->getJobID();
  }

  return last_stopped_job;
}
