#ifndef PREPARE_H
#define PREPARE_H

#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sstream>
#include <fstream>

using namespace std;

#define BUFFER_SIZE 4096 // 缓冲区大小

enum getCommandStatus{ // 获取命令的返回值
    EMPTY, NEMPTY
};

const std::string internalCommands[] = {
    "bg", "cd", "clr", "dir", "echo", "exec", "exit",
    "fg", "help", "jobs", "pwd", "set", "test", "time", "umask"
};

static int stdinFd; // 标准输入的文件描述符
static int stdoutFd; // 标准输出的文件描述符

void InitShell(char *); // 初始化shell
void PrintPrompt(); // 输出提示符
getCommandStatus GetCommand(vector<string>&); // 读入一行命令
getCommandStatus GetCommand(vector<string>&, string); 
void ExecuteCommand(vector<string>&); // 执行命令
bool InitGlobals(vector<string>&); // 初始化全局变量


#endif