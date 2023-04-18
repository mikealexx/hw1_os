#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string>
#include <vector>
#include <list>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
    // TODO: Add your data members
   public:
    const char* cmd_line;
    int args_num;
    char** args;
    Command(const char* cmd_line);
    virtual ~Command();
    virtual void execute() = 0;
    // virtual void prepare();
    // virtual void cleanup();
    //  TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
   public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
   public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
   public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
   public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    // void prepare() override;
    // void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members public:
   public:
    ChangeDirCommand(const char* cmd_line);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
   public:
    GetCurrDirCommand(const char* cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
   public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

class JobsList {
   public:
    class JobEntry {
        // TODO: Add your data members
       public:
        int job_id;
        pid_t pid;
        Command* cmd;
        const char* cmd_line;
        time_t start_time;
        bool stopped;
        JobEntry() = delete;
        JobEntry(Command* cmd, int job_id, pid_t pid, const char* cmd_line, time_t start_time, bool stopped):
            job_id(job_id), cmd(cmd), cmd_line(cmd_line), start_time(start_time), stopped(stopped) {}
        bool operator<(const JobEntry& other) const {
            return (this->pid < other.pid);
        }
        bool operator!=(const JobEntry& other) const {
            return (this->pid != other.pid);
        }
    };
    // TODO: Add your data members
   public:
    static bool comparePid(JobEntry* job1, JobEntry* job2) {
        return (job1->pid < job2->pid);
    }
    list<JobEntry*> jobs_list;
    pid_t max_job_id;
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry* getLastJob(int* lastJobId);
    JobEntry* getLastStoppedJob(int* jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

//===================================================================
//======================== Built In Commands ========================
//===================================================================

class ChpromptCommand : public BuiltInCommand {
   public:
    explicit ChpromptCommand(const char* cmd_line);
    virtual ~ChpromptCommand() {}
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
    /* Bonus */
    // TODO: Add your data members
   public:
    explicit TimeoutCommand(const char* cmd_line);
    virtual ~TimeoutCommand() {}
    void execute() override;
};

class ChmodCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    ChmodCommand(const char* cmd_line);
    virtual ~ChmodCommand() {}
    void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    GetFileTypeCommand(const char* cmd_line);
    virtual ~GetFileTypeCommand() {}
    void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    SetcoreCommand(const char* cmd_line);
    virtual ~SetcoreCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class SmallShell {
   private:
    SmallShell();

   public:
    static string prompt;  // current prompt
    static string lastWd;  // last working directory
    static JobsList* jobs_list;
    Command* CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&) = delete;      // disable copy ctor
    void operator=(SmallShell const&) = delete;  // disable = operator
    static SmallShell& getInstance()             // make SmallShell singleton
    {
        static SmallShell instance;  // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif  // SMASH_COMMAND_H_
