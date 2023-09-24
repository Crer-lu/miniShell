#ifndef EXECUTE_H
#define EXECUTE_H

#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <vector>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>


// 常规数组的大小
#define BUFFER_SIZE 4096
// 任务数量的最大值
#define MAX_JOB_COUNT 1024

using namespace std;

enum JOBSTATUS{ // 任务状态
    RUN = 0, SUSPEND, DONE
};
enum JOBTYPE{ // 任务类型
    BG = 0, FG
};
typedef struct JOB{ // 任务结构体
    pid_t pid;
    string name;
    bool isdelete;
    JOBSTATUS status;
    JOBTYPE type;
}job;
static job *jobList; // 任务列表
static int *jobCount; // 任务数量

// 是否有管道操作符
static bool ispipe;
// 是否有输入重定向
static bool isinput;
// 是否有输出重定向
static bool isoutput;
// 是否有输出重定向（追加）
static bool isoutputplus;
// 是否为后台执行命令
static bool isBg;
// 输入重定向文件路径
static string inputfile;
// 输出重定向文件路径
static string outputfile;


void ChangeDir(vector<string>&); // cd
void ClearScreen(); // clr
void ShowDir(vector<string>&); // dir
void Print(vector<string>&); // echo
void ShowHelp(vector<string>&); // help
void ShowPath(); // pwd
void ShowTime(); // time
void Set(); // set
void BackGround(vector<string>&); // bg
void FrontGround(vector<string>&); // fg
void Umask(vector<string>&); // umask
void CallExecvp(vector<string>&); // exec
void Execute(vector<string>&); // 一个socket，转接到CallExecvp
void ShowJobs(); // jobs
bool TestExp(vector<string>&); // test

void InitJobList(); // 初始化任务列表
job* CreateJob(pid_t pid, string name, JOBTYPE type, JOBSTATUS status); // 新建任务

// 信号系统初始化函数
void InitSignal();
// Sigchld:子进程结束,前台进程的终止, ctrl+c
void Sigchld( int signo, siginfo_t *info, void *context );
// Sigtstp:stop当前进程,可以将一个正在前台执行的命令放到后台,并且处于暂停状态 ctrl+z 
void Sigtstp( int signo );

void PrintBGStatus(pid_t);

#endif