#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <algorithm>
#include <string>
#include "Commands.h"


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

void _freeArguments(char** args, int args_num) {
  if(!args) {
    return;
  }
  for (int i = 0; i < args_num; i++) {
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

  _freeArguments(args, args_num);

  return flag;
}

bool _isIOCommand(const char* cmd_line) {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(cmd_line, args);

  bool flag = false;

  for (int i = 0; i < args_num; i++) {
    if (!strcmp(args[i], ">") || !strcmp(args[i], ">>")) {
      flag = true;
    }
  }

  _freeArguments(args, args_num);
  return flag;
}

bool _isBackgroundCommand(const char* cmd_line) {
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

  this->current_cmd = nullptr;
  this->smash_pid = getpid();
  this->current_pid = 0;
}

SmallShell::~SmallShell() {
  if (this->prev_dir) {
    delete[] *(this->prev_dir);
  }
  delete[] this->prev_dir;
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
  else if (_isIOCommand(cmd_line)) {
    return new RedirectionCommand(cmd_line);
  }
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
    return new TimeoutCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  this->jobs.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);

  cmd->execute();

  if (cmd->getBgCmd() == false) {
    delete cmd;
    this->current_cmd = nullptr;
  }
}

JobsList& SmallShell::getJobsList() {
  return this->jobs;
}

char** SmallShell::getLastPwd() {
  return this->prev_dir;
}

/***************************************** Timer methods *****************************************/

time_t Timer::getTotalTime() {
  if(is_running) {
    return (this->total_time + (time(nullptr) - this->last_start));
  }
  return this->total_time;
}

void Timer::stopTimer() {
  this->total_time = this->total_time + (time(nullptr) - this->last_start);
  this->is_running = false;
}

void Timer::startTimer() {
  this->last_start = time(nullptr);
  this->is_running = true;
}

/***************************************** Command methods *****************************************/

Command::Command(const char* cmd_line): timer() {
  this->cmd_line = new char[strlen(cmd_line)+1];
  strcpy(this->cmd_line, cmd_line);
  this->bg_cmd = false;
}

Command::~Command() {
  delete[] cmd_line;
}

/***************************************** Built-in commands *****************************************/

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line) {}

//chprompt
ChangePrompt::ChangePrompt(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ChangePrompt::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();

  if (args[1]) {
    strcpy(smash.getPrompt(), args[1]);
  }
    
  else {
    strcpy(smash.getPrompt(), "smash");
  }

  _freeArguments(args, args_num);
}

//showpid
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  cout << "smash pid is " << smash.getSmashPid() << endl;
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
    perror("smash error: cd: too many arguments");
  }

  else if (path && strcmp(path, "-") == 0) {
    if (!(*lastPwd)) {
      perror("smash error: cd: OLDPWD not set");
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

  _freeArguments(args, args_num);
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
  // Gets *job id* and sends a kill signal to the process
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();
  JobsList jobs = smash.getJobsList();

  char* first_param = args[1];
  char* second_param = args[2];

  if(!first_param || !second_param) {
    cerr<<"smash error: kill: invalid arguments"<<endl;
    _freeArguments(args, args_num);
    return;
  } 
  else if(first_param[0] != '-') {
    cerr<<"smash error: kill: invalid arguments"<<endl;
    _freeArguments(args, args_num);
    return;
  }//maybe we should check the number of aguments in createcommand function

  unsigned int i;
  for(i=1; i<strlen(first_param); i++) {
    if(first_param[i]<'0' || first_param[i]>'9') {
      cerr<<"smash error: kill: invalid arguments"<<endl;
      _freeArguments(args, args_num);
      return;
    }
  }

  for(i=0; i<strlen(second_param); i++) {
    if(second_param[i]<'0' || second_param[i]>'9') {
      cerr<<"smash error: kill: invalid arguments"<<endl;
      _freeArguments(args, args_num);
      return;
    }
  }
  //Should be splitted to the execute method
  int signal=stoi(string(first_param+1));//Should be checked for correction
  int job_id=stoi(string(second_param));

  JobsList::JobEntry* to_kill=jobs.getJobById(job_id);

  if(!to_kill){
		cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
    _freeArguments(args, args_num);
		return;
	}

  pid_t pid_to_kill=to_kill->getProcessID();
  if(kill(pid_to_kill, signal) == -1) {
    perror("smash error: kill failed"); //The error handling should be checked
    _freeArguments(args, args_num);
    return;
  }
  else {
    cout<<"signal number "<<signal<<" was sent to pid "<<pid_to_kill<<endl;
  }

  if(signal == SIGKILL) {
    smash.getJobsList().removeJobById(to_kill->getJobID());
  }
  if(signal == SIGSTOP) {
    to_kill->stopTheJob();
  }
  if(signal == SIGCONT) {
    to_kill->turnToBG();
  }
  _freeArguments(args, args_num);
}

//fg
ForegroundCommand::ForegroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void ForegroundCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();
  JobsList jobs = smash.getJobsList();

  if(args_num > 2) {
    cerr<<"smash error: fg: invalid arguments";
    _freeArguments(args, args_num);
    return;
  }

  JobsList::JobEntry* curr_job;
  if(args_num == 1) {
    JobsList::JobEntry* last = jobs.getLastJob(nullptr);
    if(!last) {
      cerr<<"smash error: fg: jobs list is empty"<<endl;
      _freeArguments(args, args_num);
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
        _freeArguments(args, args_num);
        return;
      }
    }

    int job_id = stoi(string(args[1]));
    JobsList::JobEntry* job_to_fg = jobs.getJobById(job_id);
    if(!job_to_fg) {
      cerr<<"smash error: fg: job-id "<<job_id<<" does not exist"<<endl;
      _freeArguments(args, args_num);
      return;
    }

    curr_job = job_to_fg;
  }

  cout<<curr_job->getCommand()->getCmdLine()<<" : "<<curr_job->getProcessID()<<endl;
  if(curr_job->isStopped()) {
    if(kill(curr_job->getProcessID(), SIGCONT) == -1) {
      perror("smash error: kill failed");
      _freeArguments(args, args_num);
      return;
    }
  }

  curr_job->resumeProcess();

  int status;
  smash.updateCurrentCmd(curr_job->getCommand());
  smash.updateCurrentPid(curr_job->getProcessID());
  //Have to handel the case that the ctrl+z is occuring here
  if(waitpid(curr_job->getProcessID(), &status, WUNTRACED) != curr_job->getProcessID()) {
    perror("smash error: waitpid failed");
  }
  smash.updateCurrentCmd(nullptr);
  smash.updateCurrentPid(0);

  jobs.removeFinishedJobs();

  _freeArguments(args, args_num);
}

//bg
BackgroundCommand::BackgroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){}

void BackgroundCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);
  SmallShell& smash = SmallShell::getInstance();
  JobsList jobs = smash.getJobsList();

  if(args_num > 2) {
    cerr<<"smash error: bg: invalid arguments";
    _freeArguments(args, args_num);
    return;
  }
  
  JobsList::JobEntry* curr_job;
  if(args_num == 1) {
    JobsList::JobEntry* last = jobs.getLastStoppedJob();
    if(!last) {
      cerr<<"smash error: bg: there is no stopped jobs to resume"<<endl;
      _freeArguments(args, args_num);
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
        _freeArguments(args, args_num);
        return;
      }
    }

    int job_id = stoi(string(args[1]));
    JobsList::JobEntry* job_to_bg = jobs.getJobById(job_id);
    if(!job_to_bg) {
      cerr<<"smash error: bg: job-id "<<job_id<<" does not exist"<<endl;
      _freeArguments(args, args_num);
      return;
    }
    if(!job_to_bg->isStopped()) {
      cerr<<"smash error: bg: job-id "<<job_id<<" is already running in the background"<<endl;
      _freeArguments(args, args_num);
      return;
    }

    curr_job = job_to_bg;
    
  }

  cout<<curr_job->getCommand()->getCmdLine()<<" : "<<curr_job->getProcessID()<<endl;

  if(curr_job->isStopped()) {
    if(kill(curr_job->getProcessID(), SIGCONT) == -1) {
      perror("smash error: kill failed");
      _freeArguments(args, args_num);
      return;
    }
  }
  //Problems with the timer
  curr_job->resumeProcess();
  
  _freeArguments(args, args_num);
}

//quit
QuitCommand::QuitCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}

void QuitCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  char* args[COMMAND_MAX_ARGS+1];
  _parseCommandLine(this->getCmdLine(), args);

  if(strcmp(args[1], "kill") == 0) {
    smash.getJobsList().killAllJobs();
  }

  kill(getpid(),SIGKILL);
}

/***************************************** Externals commands *****************************************/

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  char* command_line = new char[(strlen(this->getCmdLine())+1)];
  strcpy(command_line, this->getCmdLine());

  if (_isBackgroundCommand(this->getCmdLine())) {
    _removeBackgroundSign(command_line);
  }

  pid_t p = fork();

	if (p == 0) {
    setpgrp();
    //Maybe it's better to free the memory of the "clean" address
    const char* args[] = {"/bin/bash", "-c", command_line, NULL};
    execv(args[0], (char**)args);
	}
  else {
    if (_isBackgroundCommand(this->getCmdLine())) {
      smash.getJobsList().addJob(this, p);
    }
    else {
      smash.updateCurrentPid(p);
      smash.updateCurrentCmd(this);
      waitpid(p, nullptr, WUNTRACED);
      smash.updateCurrentPid(0);
      smash.updateCurrentCmd(nullptr);
    }
  }
  delete[] command_line;
}

/***************************************** Special commands *****************************************/

//pipe
PipeCommand::PipeCommand(const char* cmd_line): Command(cmd_line), left_cmd(new char[COMMAND_ARGS_MAX_LENGTH]),
                                            right_cmd(new char[COMMAND_ARGS_MAX_LENGTH]), pipe_operator(new char[3]) {
  const string str(cmd_line);
  _trim(str);

  size_t op1 = str.find("|&");

  if (op1 != std::string::npos) {
    strcpy(pipe_operator, "|&");
  }
  else {
    strcpy(pipe_operator, "|");
  }

  size_t left_cmd_end = str.find_first_of(pipe_operator);
  strcpy(left_cmd, str.substr(0, left_cmd_end).c_str());

  const string sub_str(str.substr(left_cmd_end + strlen(pipe_operator)));

  strcpy(right_cmd, _trim(sub_str).c_str());

  if (_isBackgroundCommand(left_cmd)) {
    _removeBackgroundSign(left_cmd);
  }
  if (_isBackgroundCommand(right_cmd)) {
    _removeBackgroundSign(right_cmd);
  }
}

PipeCommand::~PipeCommand() {
  delete[] left_cmd;
  delete[] right_cmd;
  delete[] pipe_operator;
}

void PipeCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  int fd[2];
  pipe(fd);

  pid_t p1;
  pid_t p2;

  if ((p1 = fork()) == 0) {
    // first child
    setpgrp();
    if (strcmp(this->pipe_operator, "|&") == 0) {
      dup2(fd[1],2);
    }
    else if (strcmp(this->pipe_operator, "|") == 0){
      dup2(fd[1],1);
    }
    close(fd[0]);
    close(fd[1]);
    smash.executeCommand(this->left_cmd);
    exit(0);
  }
  if ((p2 = fork()) == 0) {
    // second child 
    setpgrp();
    dup2(fd[0],0);
    close(fd[0]);
    close(fd[1]);
    smash.executeCommand(this->right_cmd);
    exit(0);
  }
  close(fd[0]);
  close(fd[1]);
  waitpid(p1, nullptr, 0);
  waitpid(p2, nullptr, 0);
}

//IO
RedirectionCommand::RedirectionCommand(const char* cmd_line): Command(cmd_line), command(new char[COMMAND_ARGS_MAX_LENGTH]),
                                            output_file(new char[COMMAND_ARGS_MAX_LENGTH]), io_operator(new char[3]) {
  const string str(cmd_line);
  _trim(str);

  size_t op1 = str.find(">>");

  if (op1 != std::string::npos) {
    strcpy(io_operator, ">>");
  }
  else {
    strcpy(io_operator, ">");
  }

  size_t command_end = str.find_first_of(io_operator);
  strcpy(command, str.substr(0, command_end).c_str());

  const string sub_str(str.substr(command_end + strlen(io_operator)));

  strcpy(output_file, _trim(sub_str).c_str());
}

void RedirectionCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  pid_t pid = fork();

  if (pid == 0) {
    setpgrp();
    close(1);
    if (strcmp(this->io_operator, ">>") == 0) {
      open(this->output_file, O_CREAT | O_WRONLY | O_APPEND, S_IRWXU | S_IRWXG);
    }
    else if (strcmp(this->io_operator, ">") == 0){
      open(this->output_file, O_CREAT | O_WRONLY, S_IRWXU  | S_IRWXG);
    }
    smash.executeCommand(this->command);
    exit(0);
  }
  else {
    waitpid(pid, nullptr, 0);
  }
}

RedirectionCommand::~RedirectionCommand() {
  delete[] command;
  delete[] output_file;
  delete[] io_operator;
}

//tail
TailCommand::TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line), path(nullptr), 
                                              lines_rd(10), total_lines(0) {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);

  if (args_num >= 2) {
    if (args_num == 3) {
      path = new char[strlen(args[2]) + 1];
      strcpy(path, args[2]);
      lines_rd = abs(stoi(string(args[1])));
    }
    else {
      path = new char[strlen(args[1]) + 1];
      strcpy(path, args[1]);
    }
  }
  
  _freeArguments(args, args_num);

  int fd = open(this->path, O_RDONLY);
  char line[1];

  while (read(fd, line, 1) > 0) {
    if (strcmp(line, "\n") == 0) {
      ++total_lines;
    }
  }

  close(fd);

  if (total_lines < lines_rd) {
    lines_rd = total_lines;
  }
}

void TailCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);

  if (args_num > 3 || args_num < 2) {
    cout << "smash error: tail: invalid arguments" << endl;
    _freeArguments(args, args_num);
    return;
  }

  else if (args_num == 3 && stoi(string(args[1])) > 0) {
    cout << "smash error: tail: invalid arguments" << endl;
    _freeArguments(args, args_num);
    return;
  }

  _freeArguments(args, args_num);

  int fd = open(this->path, O_RDONLY);
  if (fd == -1) {
    perror("smash error: tail");
    return;
  }
  char line[1];

  int start_line = this->total_lines - this->lines_rd;
  int line_iter = 0;

  while (read(fd, line, 1) > 0) {
    if (line_iter >= start_line) {
      if (write(1, line, 1) == -1) {
        perror("smash error: tail");
        return;
      }
    }
    if (strcmp(line, "\n") == 0) {
      ++line_iter;
    }
  }

  if (close(fd) == -1) {
    perror("smash error: tail");
    return;
  }
}

TailCommand::~TailCommand() {
  delete[] path;
}

//touch
TouchCommand::TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line), file_name(nullptr), timestamp(time(NULL)) {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);

  if (args_num == 3) {
    file_name = new char[strlen(args[1]) + 1];
    strcpy(file_name, args[1]);

    std::string time_str = string(args[2]);
    std::replace(time_str.begin(), time_str.end(), ':', ' ');
    char* time_args[COMMAND_MAX_ARGS+1];
    int time_args_num = _parseCommandLine(time_str.c_str(), time_args);

    tm ts;
    ts.tm_sec = stoi(time_args[0]);
    ts.tm_min = stoi(time_args[1]);
    ts.tm_hour = stoi(time_args[2]);
    ts.tm_mday = stoi(time_args[3]);
    ts.tm_mon = stoi(time_args[4]) - 1;
    ts.tm_year = stoi(time_args[5]) - 1900;
    ts.tm_wday = 0;
    ts.tm_yday = 0;
    ts.tm_isdst = 0;
    timestamp = mktime(&ts);

    _freeArguments(time_args, time_args_num);
  }

  _freeArguments(args, args_num);
}

void TouchCommand::execute() {
  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);
  _freeArguments(args, args_num);

  if (args_num != 3) {
    cout << "smash error: touch: invalid arguments" << endl;
    return;
  }

  utimbuf* ts = new utimbuf;
  ts->actime = timestamp;
  ts->modtime = timestamp;

  if (utime(file_name, ts) == -1) {
    perror("smash error: touch");
  }

  delete ts;
}

TouchCommand::~TouchCommand() {
  delete[] file_name;
}

//timeout
TimeoutCommand::TimeoutCommand(const char* cmd_line) : Command(cmd_line) {}

void TimeoutCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  char* args[COMMAND_MAX_ARGS+1];
  int args_num = _parseCommandLine(this->getCmdLine(), args);

  int seconds = stoi(args[1]);

  size_t cut_point = string(this->getCmdLine()).find(args[2]);
  string cmd_to_run = string(this->getCmdLine()).substr(cut_point);

  _freeArguments(args, args_num);

  if(_isBackgroundCommand(this->getCmdLine())) {
    cmd_to_run.erase(cmd_to_run.find('&'), 1);

    pid_t p = fork();
    if(p < 0) { //Error
      perror("smash error: fork failed");
    }
    else if(p == 0) { // Child
      setpgrp();
      char* cmd_to_execute = new char[cmd_to_run.length() + 1];
      strcpy(cmd_to_execute, cmd_to_run.data());
      const char* args[4] = {"/bin/bash", "-c", cmd_to_execute, nullptr};

      execv(args[0], (char**)args);
    }
    else { // Parent
      if(waitpid(p, nullptr, WNOHANG) != -1) {
        smash.getJobsList().addJob(this, p);
      }

      alarm(seconds);
    }
  }
  else {
    pid_t p = fork();

    if(p < 0) { // Error
      perror("smash error: fork failed");
    }
    else if(p == 0) { // Child
      setpgrp();
      char* cmd_to_execute = new char[cmd_to_run.length() + 1];
      strcpy(cmd_to_execute, cmd_to_run.data());
      const char* args[4] = {"/bin/bash", "-c", cmd_to_execute, nullptr};

      execv(args[0], (char**)args);
    }
    else { // Parent
      smash.updateCurrentPid(p);
      smash.updateCurrentCmd(this);

      alarm(seconds);
      waitpid(p, nullptr, WUNTRACED);

      smash.updateCurrentPid(0);
      smash.updateCurrentCmd(nullptr);
    }
  }
}
//We suggest you save all your timed commands in a list with a timestamp duration and pid.

/***************************************** JobsList methods *****************************************/

JobsList::JobsList(): jobs_vector(vector<JobEntry*>()), max_id(1) {}

JobsList::~JobsList() {}

void JobsList::addJob(Command* cmd, pid_t child_pid, bool isStopped) {
    int new_job_index = this->max_id++;
  
    JobEntry* new_job = new JobEntry(new_job_index, child_pid, isStopped, cmd);

    jobs_vector.push_back(new_job);
    cmd->setBgCmd();

    if(isStopped) {
      new_job->stopProcess();
    }
}

void JobsList::printJobsList() {
  for (JobEntry* iter : jobs_vector) {
    cout << "[" << iter->getJobID() << "] " << iter->getCommand()->getCmdLine() << " : " << iter->getProcessID() << " " << iter->returnDiffTime() << " secs";
    if (iter->isStopped()) {
      cout << " (stopped)";
    }
    cout << endl;
  }
}

void JobsList::killAllJobs() {
  // if( this->jobs_vector.empty()) {
  //   return;
  // }

  cout<<"smash: sending SIGKILL signal to "<< this->jobs_vector.size() << " jobs:" <<endl;

  for(JobEntry* iter : jobs_vector) {
    if(kill(iter->getProcessID(), SIGKILL) == -1) {
      cerr<<"error: kill signal failed";
    }
    else {
      int status;
      if(waitpid(iter->getProcessID(), &status, 0) != iter->getProcessID()) {
        cerr<<"error: waitpid failed";
      }
      cout << iter->getProcessID() << ": " << iter->getCommand()->getCmdLine() << endl;
    }
  }
  jobs_vector.clear();
  max_id=1;
}

void JobsList::removeFinishedJobs() {
  int status = -1;
  for(JobEntry* iter : jobs_vector) {
    waitpid(iter->getProcessID(), &status, WNOHANG);
    if (WIFEXITED(status)) {
      this->removeJobById(iter->getJobID());
    }
  }

  if(jobs_vector.empty()) {
    max_id=1;
  }
  else {
    max_id=jobs_vector.back()->getJobID() + 1;
  }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
  for(JobEntry* iter : jobs_vector) {
    if(iter->getJobID() == jobId) {
      return iter;
    }
  }

  return nullptr;
}

void JobsList::removeJobById(int jobId) {
  for(std::vector<JobEntry*>::iterator iter=jobs_vector.begin(); iter!=jobs_vector.end();iter++) {
    if((*iter)->getJobID() == jobId) {
      jobs_vector.erase(iter);
      //delete *iter; // Problematic
      break;
    }
  }
  if (jobs_vector.empty()) {
    max_id = 1;
  }
  else {
    max_id = jobs_vector.back()->getJobID() + 1;
  }
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
  if(jobs_vector.empty()) {
    return nullptr;
  }
  
  if(lastJobId) {
    *lastJobId = jobs_vector.back()->getJobID();
  }

  return jobs_vector.back();

}

JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId) {
  JobEntry* last_stopped_job = nullptr;
  for(JobEntry* iter : jobs_vector) {
    if(iter->isStopped()) {
      last_stopped_job = iter;
    }
  }

  if(last_stopped_job && jobId) {
    *jobId = last_stopped_job->getJobID();
  }

  return last_stopped_job;
}

JobsList::JobEntry* JobsList::jobExistsByPID(pid_t pid) {
  for(JobsList::JobEntry* iter:this->jobs_vector) {
    if(iter->getProcessID() == pid) {
      return iter;
    }
  }

  return nullptr;
}


time_t JobsList::JobEntry::returnDiffTime() { 
  if(this->is_stopped) {
    return job_time;
  }
  return this->cmd->getRunningTime();
}

void JobsList::JobEntry::stopProcess() {
  this->is_stopped = true;
  this->cmd->stopCommand();
}

void JobsList::JobEntry::resumeProcess() {
  this->is_stopped = false;
  this->cmd->startCommand();
}
