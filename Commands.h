#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string>
#include <vector>
#include <map>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
    // TODO: Add your data members
   public:
    const char* cmd_line;
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
    QuitCommand(const char* cmd_line);
    virtual ~QuitCommand() {};
    void execute() override;
};

class JobsList {
   public:
    class JobEntry {
        // TODO: Add your data members
       public:
        pid_t pid;
        char* og_cmd_line;
        time_t start_time;
        bool stopped;
        //bool fg;
        JobEntry() = delete;
        JobEntry(pid_t pid, char* cmd_line, time_t start_time, bool stopped):
            pid(pid), og_cmd_line(cmd_line), start_time(start_time), stopped(stopped) {}
    };
    // TODO: Add your data members
   public:
    map<int ,JobEntry*> jobs_map; //map job id to job entry
    int max_job_id;
    JobsList();
    ~JobsList();
    void addJob(pid_t pid, JobEntry* job, bool isStopped = false);
    void printJobsList();
    void deleteJobs();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry* getJobById(int jobId);
    void removeJobById(int jobId);
    bool empty();
    JobEntry* getJobByPid(pid_t pid);
    //JobEntry* getLastJob(int* lastJobId);
    JobEntry* getLastStoppedJob();
    int getJobIdByPid(pid_t pid);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class AlarmList {
    public:
        class AlarmEntry {
            public:
                string cmd_line;
                time_t timestamp;
                int duration;
                pid_t pid;

                AlarmEntry(string cmd_line, time_t timestamp, int duration, pid_t pid):
                    cmd_line(cmd_line), timestamp(timestamp), duration(duration), pid(pid) {}
                ~AlarmEntry() = default;
        };
    
        vector<AlarmEntry*> alarm_list;
        AlarmList();
        ~AlarmList();
        void addAlarm(string cmd_line, time_t timestamp, int duration, pid_t pid);
        void removeFinishedAlarms(time_t time);
        int lowestTime(time_t time);
        void removeAlarmByPid(pid_t pid);
        AlarmEntry* getCurrAlarm(time_t time);
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
    JobsCommand(const char* cmd_line);
    virtual ~JobsCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    ForegroundCommand(const char* cmd_line);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
   public:
    BackgroundCommand(const char* cmd_line);
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
    KillCommand(const char* cmd_line);
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
    static AlarmList* alarm_list;
    static pid_t curr_pid;
    static char* curr_cmd_line;
    static bool fg;
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
