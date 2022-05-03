#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
  
  char* cmd_line;

 public:
  
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  
  const char* getCmdLine() { return this->cmd_line; }
  //virtual void prepare();
  //virtual void cleanup();
};



/***************************************** Jobs class *****************************************/

class JobsList {
 public:
  class JobEntry {
        int job_id;
        pid_t process_id;
        bool is_stopped;//Msybe enum for status will be more reasonable
        bool BG;
        bool finished;
        Command* cmd;
        time_t job_time;
    
    public:
        JobEntry(int job_id, int process_id, bool is_stopped, Command* cmd) : job_id(job_id), 
            process_id(process_id), is_stopped(is_stopped), BG(false), finished(false), cmd(cmd), job_time(time(NULL)) {}
        ~JobEntry() { delete cmd; }
        int getJobID() { return job_id; }
        pid_t getProcessID() { return process_id; }
        bool isStopped() { return is_stopped; }
        void stopTheJob() { is_stopped=true; }
        void turnToBG() { BG=true; }
        void turnToFG() { BG=false; }
        Command* getCommand() { return cmd; }
        time_t returnDiffTime() { return difftime(time(NULL), job_time); }
  };
 
  std::vector<JobEntry*> jobs_vector;
  int max_id;
 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, pid_t child_pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId = nullptr);
};


/***************************************** Built-in commands *****************************************/

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};


//chprompt
class ChangePrompt : public BuiltInCommand {
 public:
  ChangePrompt(const char* cmd_line);
  virtual ~ChangePrompt() {}
  void execute() override;
};


//cd
class ChangeDirCommand : public BuiltInCommand {
 public:
  ChangeDirCommand(const char* cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};


//pwd
class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};


//showpid
class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};


//jobs
class JobsCommand : public BuiltInCommand {
  JobsList* list;
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};


//kill
class KillCommand : public BuiltInCommand {
  int job_to_kill;
 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};


//fg
class ForegroundCommand : public BuiltInCommand {
 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};


//bg
class BackgroundCommand : public BuiltInCommand {
 public:
  BackgroundCommand(const char* cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};


//quit
class QuitCommand : public BuiltInCommand {
 public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};


/***************************************** Special commands *****************************************/

//pipe
class PipeCommand : public Command {
  char* left_cmd;
  char* right_cmd;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand();
  void execute() override;
};


//IO
class RedirectionCommand : public Command {
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};


//tail
class TailCommand : public BuiltInCommand {
 public:
  TailCommand(const char* cmd_line);
  virtual ~TailCommand() {}
  void execute() override;
};


//touch
class TouchCommand : public BuiltInCommand {
 public:
  TouchCommand(const char* cmd_line);
  virtual ~TouchCommand() {}
  void execute() override;
};

/***************************************** SmallShell *****************************************/
class SmallShell {
 private:
  char* prompt;
  char** prev_dir;
  JobsList jobs;

  Command* current_cmd;
  pid_t current_pid;

  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  void updateCurrentCmd(Command* cmd) { this->current_cmd = cmd; }
  void updateCurrentPid(pid_t pid) { this->current_pid = pid; }
  Command* getCurrCmd() { return current_cmd; }
  pid_t getCurrentPid() { return current_pid; }
  JobsList& getJobsList();
  void executeCommand(const char* cmd_line);
  char* getPrompt() const;
  char** getLastPwd();
  ~SmallShell();
};

/***************************************** Externals command *****************************************/
class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

#endif //SMASH_COMMAND_H_
