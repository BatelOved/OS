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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell(): prev_dir((char**)malloc(COMMAND_ARGS_MAX_LENGTH)) {
  *(this->prev_dir) = nullptr;
}

SmallShell::~SmallShell() {
  free(this->prev_dir);
}

JobsList::JobsList() {

}

JobsList::~JobsList() {
  
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  char** args = (char**)malloc(COMMAND_ARGS_MAX_LENGTH);
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
    return new ChangePrompt(cmd_line, args[1]);
  }
  
  else {
    return new ExternalCommand(cmd_line);
  }
  free(args);
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char* cmd_line) {

}

Command::~Command() {

}

BuiltInCommand::BuiltInCommand(const char* cmd_line): Command(cmd_line) {

}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {

}

void ExternalCommand::execute() {
  
}

ChangePrompt::ChangePrompt(const char* cmd_line, const char* prompt_name): BuiltInCommand(cmd_line) {
  SmallShell& smash = SmallShell::getInstance();

  if (name) {
    this->name = (char*)malloc(sizeof(name)+1);
    strcpy(this->name, name);
  }
  else
    this->name = (char*)malloc(sizeof("smash")+1);
    strcpy(this->name, "smash");
}

void ChangePrompt::execute() {
    // while(true) {
    //     std::cout << this->name << "> ";
    //     std::string cmd_line;
    //     std::getline(std::cin, cmd_line);
    //     smash.executeCommand(cmd_line.c_str());
    // }
}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd): BuiltInCommand(cmd_line), 
            curr_dir((char*)malloc(COMMAND_ARGS_MAX_LENGTH)), prev_dir((char*)malloc(COMMAND_ARGS_MAX_LENGTH)), 
                                                                      path((char*)malloc(COMMAND_ARGS_MAX_LENGTH)) {
  char** args = (char**)malloc(COMMAND_ARGS_MAX_LENGTH);
  string cmd_s = _trim(string(cmd_line));
  _parseCommandLine(cmd_line, args);

  if (args[2])
    perror("smash error: cd: too many arguments");

  if (!(*plastPwd)) {
    if (strcmp(args[1], "-") == 0)
      perror("smash error: cd: OLDPWD not set");
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

void ChangeDirCommand::execute() { // TODO: Add error handling
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

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line) {
  
}

void GetCurrDirCommand::execute() {
  char* buff = (char*)malloc(COMMAND_ARGS_MAX_LENGTH);
  cout << getcwd(buff, COMMAND_ARGS_MAX_LENGTH) << endl;
  free(buff);
}

ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {

}

void ShowPidCommand::execute() {
  cout << "smash pid is " << getpid() << endl;
}

