#include "prepare.h"
#include "execute.h"

// 辅助函数：解析命令行参数
vector<string>ParseCommand(string& command){
    std::vector<std::string> args; // 创建参数容器
    std::istringstream arg_string(command); // 创建istringstream对象，用于切分
    std::string arg; // 被切分出的每个参数，暂存在arg

    while (arg_string >> std::ws >> arg) {
        args.push_back(arg); // 把所有的参数压入args中
    }

    return args;
}

// 辅助函数：用来判断是哪条指令
int FindIndex(string& command){
    // 在internalCommand里寻找与之匹配的命令 返回index
    for(int i = 0; i < 15; ++ i){
        if(command == internalCommands[i]) return i;
    }
    return -1;
}

void InitShell(char *chpath){

    // 1. myshell环境变量

    for(int i = strlen(chpath) - 1; i >= 0; -- i)
        if(chpath[i] == '/'){ // 首先把打开可执行文件时的相对路径进行调整
            chpath[i] = 0; 
            break;
        }
    chdir(chpath); // 把路径调整到这个位置
    char path[BUFFER_SIZE];
    getcwd(path, BUFFER_SIZE);
    strcat(path, "/myshell"); // 构建出环境变量
    setenv("myshell", path, 1);

    // 2. 一些全局变量

    // 是否有管道操作符
    ispipe = false;
    // 是否有输入重定向
    isinput = false;
    // 是否有输出重定向
    isoutput = false;
    // 是否有输出重定向（追加）
    isoutputplus = false;
    // 是否为后台执行命令
    isBg = false;

    // 3. 初始化jobList
    InitJobList();

    // 4. 初始化signal
    InitSignal();
}

void PrintPrompt(){
    // 获取path参数
    char path[BUFFER_SIZE];
    getcwd(path, BUFFER_SIZE);
    char hostname[BUFFER_SIZE];
    gethostname(hostname, BUFFER_SIZE);
    // 输出构建出的shell提示符
    printf("%s@%s:%s$ ", getlogin() ,hostname, path);
}

getCommandStatus GetCommand(vector<string>& args, string line){
    args = ParseCommand(line); 
    // 如果命令行参数个数为0 那么返回值反馈一下
    return args.size() == 0 ? getCommandStatus::EMPTY : getCommandStatus::NEMPTY;
}

getCommandStatus GetCommand(vector<string>& args){
    // 获取一行命令
    string input;
    getline(cin, input);
    // 切分命令与参数
    args = ParseCommand(input);
    // 如果命令行参数个数为0 那么返回值反馈一下
    return args.size() == 0 ? getCommandStatus::EMPTY : getCommandStatus::NEMPTY;
}

//帮助我们处理重定向及后台程序
bool InitGlobals(vector<string>& args){
    // 是否有管道操作符
    ispipe = false;
    // 是否有输入重定向
    isinput = false;
    // 是否有输出重定向
    isoutput = false;
    // 是否有输出重定向（追加）
    isoutputplus = false;
    // 是否为后台执行命令
    isBg = false;

    for(auto itr = args.begin(); itr != args.end();){
        if(*itr == "<"){ // stdin重定向
            isinput = true;
            args.erase(itr);
            inputfile = *itr;
            args.erase(itr);
        }
        else if(*itr == ">"){ // stdout重定向
            isoutput = true;
            args.erase(itr);
            outputfile = *itr;
            args.erase(itr);
        }
        else if(*itr == ">>"){ // 追加
            isoutputplus = true;
            args.erase(itr);
            outputfile = *itr;
            args.erase(itr);
        }
        else if(*itr == "|"){ // 管道
            ispipe = true;
            itr ++;
        }
        else if(*itr == "&"){ // 后台命令
            isBg = true;
            args.erase(args.end() - 1);
            break;
        }
        else{
            itr ++;
        }
    }
    // 先备份标准输入输出文件符，最后要改回来
    stdinFd = dup(fileno(stdin));
    stdoutFd = dup(fileno(stdout));
    //cout<< inputfile<< " "<< outputfile << endl;
    if(ispipe){
        bool appear = false;
        vector<string>args_pip1;
        vector<string>args_pip2;
        for(auto itr = args.begin(); itr != args.end(); itr ++){
            if(*itr == "|"){
                appear = true;
                continue;
            }
            if(appear){
                args_pip2.push_back(*itr);
            }
            else{
                args_pip1.push_back(*itr);
            }
        }
        int pipeFile[2]; // 1用来写，0用来读
        // 创建管道，并将管道两端的文件描述符保存到pipeFile中，失败则错误退出
        if( pipe( pipeFile ) == -1 ){
            printf("pip create fail!");
            exit(1);
        }
        pid_t pid_l;
        pid_l = fork();
        if(pid_l == 0){ 
            // 把管道输出关闭
            close(pipeFile[0]);
            // 把标准输出重定向到管道输入
            dup2(pipeFile[1], STDOUT_FILENO );
            ExecuteCommand(args_pip1);
            exit(0);
        }
        else if(pid_l > 0){
            // 调用waitpid函数等待管道左端输入完毕
            waitpid( pid_l, NULL, WUNTRACED);
            close(pipeFile[1]); //关闭管道输入端，这里很重要，如果放在子进程close，将会无法停止
            pid_t pid_r;
            pid_r = fork();
            if(pid_r == 0){
                // 把输出端重定向
                dup2( pipeFile[0], STDIN_FILENO );
                ExecuteCommand(args_pip2);
                exit(0);
            }
            else if(pid_r > 0){
                // 用waitpid函数等待子进程结束
                wait(NULL);
                close(pipeFile[0]);
                close(pipeFile[1]);
            }
        }   
        return false;
    }
    if(isoutput){
        // 将对应的文件打开，标识符存到outputFile中
        // O_RDWR表示读写打开，O_TRUNC表示存在则刷新，O_CREAT表示不存在则创建
        int fileDescriptor = open( outputfile.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
        // 将标准输出重定向，失败就报错
        // 用fileno函数把文件流指针转换成文件描述符
        if( dup2( fileDescriptor, fileno( stdout ) ) == -1){
            printf("stdout change wrong\n");
        }
        // 关闭文件
        close( fileDescriptor );
    }
    if(isoutputplus){
        // 将对应的文件打开，标识符存到outputFile中
        // O_RDWR表示读写打开，O_APPEND表示追加，O_CREAT表示不存在则创建
        int fileDescriptor = open( outputfile.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
        // 将标准输出重定向，失败就报错
        // 用fileno函数把文件流指针转换成文件描述符
        if( dup2( fileDescriptor, fileno( stdout ) ) == -1){
            printf("stdin change wrong\n");
        }
        // 关闭文件
        close( fileDescriptor );
    }
    if(isinput){
        // 将对应的文件打开，标识符存到inputFile中
        // O_RDONLY代表只读取
        int fileDescriptor = open( inputfile.c_str(), O_RDONLY, 0666);
        // 将标准输入重定向，失败就报错
        // 用fileno函数把文件流指针转换成文件描述符
        if( dup2( fileDescriptor, fileno( stdin ) ) == -1){
            printf("stdin change wrong\n");
        }
        // 关闭文件
        close( fileDescriptor );
    }
    return true;
}

void RestoreGlobals(){
    if(isoutput || isoutputplus){
     // 恢复标准输出
        dup2(stdoutFd, fileno(stdout));
        close(stdoutFd);
    }
    if(isinput){
        // 恢复标准输入
        dup2(stdinFd, fileno(stdin));
        close(stdinFd);
    }
}

// 辅助函数：把string的vector转化为一个字符串
string ToString(vector<string>& args){
    string ret = ""; // 空字符串
    for(int i = 0; i < args.size(); ++ i){
        ret += args[i]; // 逐个加上vector里的元素
        ret += " ";
    }
    return ret;
}


void ExecuteCommand(vector<string>& args){
    int index = FindIndex(args[0]);
    switch (index)
    {
    case 0: // bg
        BackGround(args);
        break;
    case 1: // cd
        ChangeDir(args);
        break;
    case 2: // clr
        ClearScreen();
        break;
    case 3: // dir
        ShowDir(args);
        break;
    case 4: // echo
        Print(args);
        break;
    case 5: // exec
        Execute(args);
        break;
    case 6: // exit
        cout<< "Exit from myshell"<< endl;
        exit(0);
        break;
    case 7: // fg
        FrontGround(args);
        break;   
    case 8: // help
        ShowHelp(args);
        break;   
    case 9: // jobs
        ShowJobs();
        break;   
    case 10: // pwd
        ShowPath();
        break;   
    case 11: // set
        Set();
        break;   
    case 12: // test
        if(TestExp(args)) cout<< "TRUE"<< endl;
        else    cout<< "FALSE"<< endl;
        break; 
    case 13: // time 
        ShowTime();
        break;      
    case 14: // umask
        Umask(args);
        break;   
    default:
        // 输入的指令不在自己实现的范围之内
        // 进行系统调用
        pid_t pid = fork();
        if(pid < 0){
            cout<< "Fork error"<< endl;
            exit(1);
        }
        else if(pid == 0){
            char *parent_path = getenv("myshell");
            // 调用setenv设置parent环境变量
            setenv( "parent", parent_path, 1);
            // if(isinput){
            //     args.push_back(inputfile);
            // }
            CallExecvp(args);
            exit(0);// 退出子进程
        }
        else{
            if(isBg == true){
                //如果是后台指令
                CreateJob(pid, ToString(args), BG, RUN);
                waitpid( pid, NULL, WNOHANG); //WNOHANG 表明不阻塞主进程
                //输出一下当前bg状态的数目
                PrintBGStatus(pid);
            }
            else{
                CreateJob(pid, ToString(args), FG, RUN);
                waitpid( pid, NULL, WUNTRACED); //WUNTRACED 表明阻塞主进程
            }
        }
        break;
    }
    RestoreGlobals();
}
