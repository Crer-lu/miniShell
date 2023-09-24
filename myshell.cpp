#include <stdio.h>
#include "prepare.h"
#include <fstream>

int main(int argc, char **argv){
    InitShell(argv[0]);
    vector<string> args;
    if(argc != 1){
        //后面跟着文件
        ifstream infile(argv[1]);
        if(!infile){
            //打开失败
            cout<< "Failure in opening file"<< endl;
            exit(1);
        }
        string line;
        while(infile >> line){ // 逐行读入
            args.clear();
            GetCommand(args, line);
            ExecuteCommand(args);
        }
        exit(0);
    }
    // myshell后面没有跟上文件,就是交互式的处理命令
    while(true){   
        PrintPrompt(); // 输出提示符
        args.clear(); // 清空参数列表
        if(GetCommand(args) == getCommandStatus::EMPTY) continue;
        if(InitGlobals(args))
            ExecuteCommand(args); // 执行命令
    }
    return 0;
}