#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


/***************************************** Timer class *****************************************/

// A class for counting the running time for each command
class Timer {
  time_t total_time; // The actual time that passed
  time_t last_start; // The last time we started the timer

  bool is_running; // A flag for checking if the timer is running

public:

  Timer() : total_time(0) , last_start(time(nullptr)), is_running(true) {}
  ~Timer() = default;

  time_t getTotalTime();
  void stopTimer();
  void startTimer();
};

/***************************************** Command class *****************************************/

class Command {
  
  char* cmd_line;
  Timer timer;

 public:
  
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  
  const char* getCmdLine() { return this->cmd_line; }
  void stopCommand() { this->timer.stopTimer(); }
  void startCommand() { this->timer.startTimer(); }
  time_t getRunningTime() { return this->timer.getTotalTime(); }
};

/***************************************** TimeoutObject commands *****************************************/

// A class for managing the timeout commands in the correct order
class TimeoutObject {
  
  pid_t pid;

  pid_t start_counting_point;
  time_t time_left;
  time_t time_to_alarm;

  Command* command;

public:

  TimeoutObject(pid_t pid, time_t time_to_alarm, Command* cmd) : pid(pid), start_counting_point(time(nullptr)),
      time_left(time_to_alarm), time_to_alarm(time_to_alarm), command(cmd) {}
  
  ~TimeoutObject() = default;

  pid_t getPID() { return pid; }
  void updateTimeLeft(time_t current_time);
  time_t getTimeLeft() { return time_left; }
  Command* getCommand() { return command; }

};

/***************************************** Jobs class *****************************************/

class JobsList {
 public:
  class JobEntry {
        int job_id;
        pid_t process_id;
        bool is_stopped;
        Command* cmd;

        time_t job_time;
    
    public:
        JobEntry(int job_id, int process_id, bool is_stopped, Command* cmd) : job_id(job_id), 
            process_id(process_id), is_stopped(is_stopped), cmd(cmd), job_time(0) {}
        ~JobEntry() { }
        int getJobID() { return job_id; }
        pid_t getProcessID() { return process_id; }
        bool isStopped() { return is_stopped; }
        Command* getCommand() { return cmd; }
        time_t returnDiffTime();
        void stopProcess();
        void resumeProcess();
  };
 
  std::vector<JobEntry*> jobs_vector;
  std::vector<JobEntry*> stopped_jobs_vector;
  std::list<TimeoutObject*> timeout_list;

  int max_id;

 public:
  JobsList();
  ~JobsList();
  void addJob(Command* cmd, pid_t child_pid, bool isStopped = false);
  void addJobToStoppedList(JobEntry* stopped_job);
  void addTimeoutObject(pid_t pid, time_t time_to_run, Command* cmd);

  // Sends the timeout that caused the alarm signal
  TimeoutObject* getCurrentTimeout(time_t current_time);

  // Creates a new alarm call for the next timeout
  void continueNextAlarm();

  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  void removeTimeoutObject(pid_t pid);
  void removeFromStoppedList(int jobId);
  JobEntry* getLastJob(int* lastJobId);
  JobEntry* getLastStoppedJob(int *jobId = nullptr);

  // Returns a pointer to the job with the specific pid(or nullptr if
  // there isn't any job with that pid)
  JobsList::JobEntry* jobExistsByPID(pid_t pid);
  
};

/***************************************** SmallShell *****************************************/
class SmallShell {
 private:
  char* prompt;
  char** prev_dir;
  JobsList jobs;
  pid_t smash_pid;

  Command* current_cmd;
  pid_t current_pid;

  time_t alarm_start;
  unsigned int last_alarm_time;

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
  pid_t getSmashPid() { return smash_pid; }
  time_t getAlarmStart() { return alarm_start; }
  void setAlarm(unsigned int seconds);
  unsigned long getLastAlarmTime() { return last_alarm_time; }
  JobsList& getJobsList();
  void executeCommand(const char* cmd_line);
  char* getPrompt() const;
  char** getLastPwd();
  ~SmallShell();
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
  char* pipe_operator;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand();
  void execute() override;
};


//IO
class RedirectionCommand : public Command {
  char* command;
  char* output_file;
  char* io_operator;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand();
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};


//tail
class TailCommand : public BuiltInCommand {
  char* path;
  int lines_rd;
  int total_lines;
 public:
  TailCommand(const char* cmd_line);
  virtual ~TailCommand();
  void execute() override;
};


//touch
class TouchCommand : public BuiltInCommand {
  char* file_name;
  time_t timestamp;
 public:
  TouchCommand(const char* cmd_line);
  virtual ~TouchCommand();
  void execute() override;
};

//timeout
class TimeoutCommand : public Command {

 public:
  TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

/***************************************** Externals command *****************************************/
class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

#endif //SMASH_COMMAND_H_
