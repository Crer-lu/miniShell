#include "execute.h"

void ChangeDir(vector<string>& args){
    if (args.size() == 1) {
        chdir(getenv("HOME")); // 只有一个参数，就跳转到home目录
    } else if (args.size() == 2) {
        // 有两个参数，就跳转到指定位置
        if(chdir(args[1].c_str()) == -1){
            cout<< "Invalid directory"<< endl;
        }
        else{
            // chdir()不会修改PWD环境变量
            char path[BUFFER_SIZE];
            getcwd(path, BUFFER_SIZE);
            setenv("PWD", path, 1);
        }
    } else {
        std::cout << "Usage: cd [directory]" << std::endl; // 多于两个参数，说明是非法输入
    }
}

void ClearScreen(){
    cout << "\033[2J\033[1;1H"; // 清屏专用格式化字符串
}

// 供父函数调用的重载子函数 仅列出当前目录下的文件
void ShowDir(string dir){
    // 这个函数其实在 UNIX 的辅导书中有例子
    // 首先定义 DIR 和 dirent 类型指针 dirp、direntp
    DIR *dirp;
    struct dirent *direntp;
    // 如果打开目录文件失败，则报错,并直接结束函数
    if( ( dirp = opendir( dir.c_str() )) == NULL ){
        cout<< "Failure in opening "<< dir.c_str() << endl;
        return;
    }
    // 循环读取目录中的文件，直到结束为止
    while ( (direntp = readdir( dirp )) != NULL){
        cout<< direntp->d_name<< " ";
    }
    // 输出一个空行分隔
    cout<< endl;
    return;
}

void ShowDir(vector<string>& args){
    if(args.size() == 1){ // 如果没有参数，就把当前目录显示
        char workdir[BUFFER_SIZE];
        getcwd(workdir, BUFFER_SIZE);
        ShowDir(workdir);
    }
    else{ // 如果有若干个参数，就逐个显示内容
        for(int i = 1; i < args.size(); ++ i){
            if(args.size() != 1)
                cout<< args[i] << ": " << endl;
            ShowDir(args[i]); // 调用子函数
        }
    }
    return ;
}
void Print(vector<string>& args){
    for(int i = 1; i < args.size(); ++ i){
        cout << args[i]; // 逐个输出参数
        if(i != args.size() - 1) cout<< " "; // 把多个空白符吞并为一个
    }
    cout<< endl;
}
void ShowHelp(vector<string>& args){
    // 获取path参数
    char *helpdir = getenv("myshell");
    // 构建出help位置
    for(int i = strlen(helpdir) - 1; i >= 0; -- i){
        if(helpdir[i] == '/'){
            helpdir[i] = 0;
            break;
        }
    }
    strcat(helpdir, "/help");
    // 从本地读取用户手册，然后输出（尚需完善）
    ifstream file(helpdir);
    string line;
    while(getline(file, line)){
        cout<< line<< endl;
    }
    return ;
}

void ShowPath(){
    // 获取path参数
    char path[BUFFER_SIZE];
    getcwd(path, BUFFER_SIZE);
    // 输出构建出的shell提示符
    cout<< path<< endl;
}

void ShowTime(){
    time_t t; // 声明时间戳变量
    time(&t); // 获取时间戳
    cout<< ctime(&t); // 格式化输出时间
}

void Set(){
    // 因为外部变量 environ 中有所有的环境变量，直接声明输出就好了
    int index = 0;
    while( environ[index] ){
        cout << environ[index]<< endl;
        index ++;
    }
}

void BackGround(vector<string>&){
    // 寻找最近被挂起的任务
    int index;
    for( index = *jobCount - 1; index > 0; -- index){
        if( jobList[index].status == SUSPEND && jobList[index].type == BG && jobList[index].pid > 0)   break;
    }

    // 判断是否找到
    // 未找到的话，就输出提示信息
    if( index == 0 ){
        printf("There is no suspend job in the backgroud!\n");
        return;
    }
    // 找到的话，就修改一下该进程的状态
    // 首先更新jobList
    jobList[index].status = RUN;
    // 用kill函数向该进程发送信号
    kill( jobList[index].pid, SIGCONT );
    // 最后输出一下提示符
    cout<< "[" << index+1 << "]" + jobList[index].name + "&\n"<< endl;
}
void FrontGround(vector<string>&){
    // 寻找最近被挂起的任务
    int index;
    for( index = *jobCount - 1; index > 0; -- index){
        if( jobList[index].status == SUSPEND && jobList[index].type == BG && jobList[index].pid > 0)   break;
    }

    // 判断是否找到
    // 未找到的话，就输出提示信息
    if( index == 0 ){
        printf("There is no suspend job in the backgroud!\n");
        return;
    }
    // 找到的话，就修改一下该进程的状态
    cout<< jobList[index].name << endl;
    //继续执行该进程
    jobList[index].type = FG;
    jobList[index].status = RUN;
    kill( jobList[index].pid, SIGCONT );
    //阻塞主进程，等待指定的进程执行完毕
    waitpid( jobList[index].pid, NULL, WUNTRACED);
}
void Umask(vector<string>& args){
    if(args.size() == 1){ // 显示原来的mask
        mode_t ori_mask;
        ori_mask = umask(0); // 取出原来的mask
        printf("%04d\n", ori_mask);
        umask(ori_mask); //修改回原来的mask
    }
    else{
        int sum = 0;
        for(int i = 0; i < args[1].size(); ++ i){ // 判断一下每一位是否符合0-7的要求
            sum *= 10;
            if(args[1][i] - '0' >= 8){
                cout<< "octal number out of range 0-7"<< endl;
                return ;
            }
            sum += args[1][i] - '0';
        }
        umask(sum);
    }
}
//辅助函数，供Execute()调用
void CallExecvp(vector<string>& args){
    char *commands[args.size() + 1];
    commands[args.size()] = NULL;
    for(int i = 0; i < args.size(); ++ i){
        commands[i] = (char *)malloc(strlen(args[i].c_str()) + 1);
        strcpy(commands[i], args[i].c_str()); // 构建命令字符串
    }
    execvp(commands[0], commands); // 调用execvp函数
}
void Execute(vector<string>& args){
    if(args.size() == 1) exit(0);
    else{
        args.erase(args.begin()); // 去掉exec，剩下的交给execvp来执行
        CallExecvp(args); // 调用子函数
    }
}
void ShowJobs(){
    char *stat[] = {"RUN", "SUSPEND", "DONE"};
    for(int i = 0; i < *jobCount; ++ i)
        if(jobList[i].isdelete == false) // 把任务列表格式化输出
            printf("[%d] %s %s\n", jobList[i].pid, stat[jobList[i].status], jobList[i].name.c_str());

}
//辅助函数，把字符串转化为数字
int ToNumber(string & arg){
    int sum = 0;
    for(int i = 0; i < arg.size(); ++ i){ // 逐个遍历字母，然后转化为数字
        sum *= 10;
        sum += arg[i] - '0';
    }
    return sum;
}
bool TestExp(vector<string>& args){
    //三种类型 数值、字符串、文件
    if(args.size() == 4){
        if(args[2] == "-eq"){ // 数字相等
            return ToNumber(args[1]) == ToNumber(args[3]);
        }
        else if(args[2] == "-ne"){ // 数字不相等
            return ToNumber(args[1]) != ToNumber(args[3]);
        }
        else if(args[2] == "-gt"){ // 数字大于
            return ToNumber(args[1]) > ToNumber(args[3]);
        }
        else if(args[2] == "-ge"){ // 数字大于等于
            return ToNumber(args[1]) >= ToNumber(args[3]);
        }
        else if(args[2] == "-lt"){ // 数字小于
            return ToNumber(args[1]) < ToNumber(args[3]);
        }
        else if(args[2] == "-le"){ // 数字小于等于
            return ToNumber(args[1]) <= ToNumber(args[3]);
        }
        else if(args[2] == "="){ // 字符串相等
            return args[1] == args[3];
        }
        else if(args[2] == "!="){ // 字符串不相等
            return args[1] != args[3];
        }
    }

    else if(args.size() == 3){
        struct stat buf;
        int ret = stat(args[2].c_str(), &buf);
        if(ret < 0) return false;
        if(args[1] == "-z"){ // 判断空字符串
            return args[2].size() == 0;
        }
        else if(args[1] == "-n"){ // 判断非空字符串
            return args[2].size() != 0;
        }
        else if(args[1] == "-e"){ // 判断是否存在文件
            return true;
        }
        else if(args[1] == "-r"){ // 判断是否为可读文件
            return buf.st_mode & S_IRUSR;
        }
        else if(args[1] == "-w"){ // 判断是否为可写文件
            return buf.st_mode & S_IWUSR;
        }
        else if(args[1] == "-x"){ // 判断是否为可执行文件
            return buf.st_mode & S_IXUSR;
        }
        else if(args[1] == "-s"){ // 非空文件
            return buf.st_size > 0;
        }
        else if(args[1] == "-d"){ // 目录文件
            return S_ISDIR(buf.st_mode);
        }
        else if(args[1] == "-f"){ // 普通文件
            return S_ISREG(buf.st_mode);
        }
        else if(args[1] == "-c"){ // 字符文件
            return S_ISCHR(buf.st_mode);
        }
        else if(args[1] == "-b"){ // 块文件
            return S_ISBLK(buf.st_mode);
        }
    }
    else{
        cout<< "Incorrect arugument!"<< endl;
        exit(1);
    }
}

void InitJobList(){
    int ret_status;
    //share memory get
    //key: 标识符的规则
    //size:共享存储段的字节数
    //flag:读写的权限
    //返回值：成功返回共享存储的id，失败返回-1
    ret_status = shmget( IPC_PRIVATE , sizeof(job)*(MAX_JOB_COUNT) + sizeof(int) , 0666 | IPC_CREAT );

    if( ret_status == -1 ){
        // 返回失败，报错
        fprintf( stderr, "[myshell] Error: Create shared memory failed!");
        exit(1);
    }

    void* job_pointer;
    //share memory at
    //shm_id是由shmget()函数返回的共享内存标识。
    //shm_addr指定共享内存连接到当前进程中的地址位置，通常为空，表示让系统来选择共享内存的地址。
    //shm_flg是一组标志位，通常为0。
    job_pointer = shmat( ret_status, 0, 0);

    if(job_pointer == (void*)-1 ){
        fprintf( stderr, "[myshell] Error: Connnect shared memory failed!");
        exit(1);
    }

    jobList = (job *)job_pointer;

    jobList[0].pid = getpid(); // 获取myshell进程的pid
    jobList[0].name = "myshell";
    jobList[0].type = JOBTYPE::FG;
    jobList[0].status = JOBSTATUS::RUN;
    for(int i = 1; i < MAX_JOB_COUNT; ++ i){
        jobList[i].pid = -1; // 其余任务的pid先设定为-1
    }
    jobCount = (int *)malloc(sizeof(int));
    *jobCount = 1; // 当前所有进程的计数
}

job* CreateJob(pid_t pid, string name, JOBTYPE type, JOBSTATUS status){
    // 获取已有job数量
    int index = *jobCount;
    // 判断是否已经到达上限
    if( index == MAX_JOB_COUNT){
        fprintf( stderr, "[myshell] Error: Job has reached the max number!");
        exit(1);
    }
    // 添加新任务
    (*jobCount)++; // 数量++
    // 设置各个属性
    jobList[index].pid = pid; 
    jobList[index].name = name;
    jobList[index].type = type;
    jobList[index].status = status;
    jobList[index].isdelete = false;
    // 返回内存地址
    return (&jobList[index]);
}

// 信号系统初始化函数
void InitSignal(){
    // 进程停止或者终止时信号的处理结构
    struct sigaction sigchldAction;
    // 挂起信号的处理结构
    struct sigaction sigtstpAction;

    // 首先将sigchldAction变量初始化一下
    memset( &sigchldAction, 0, sizeof(sigchldAction));
    // 修改sigchldAction的sa_flags
    // 设置SA_RESTART和SA_SIGINFO两个flag
    sigchldAction.sa_flags = SA_SIGINFO | SA_RESTART;
    // 设置sigchidAction的函数地址为自定义的函数地址
    sigchldAction.sa_sigaction = Sigchld;
    // 重置sigchidAction的sa_mask
    sigemptyset( &sigchldAction.sa_mask );
    // 调用sigaction函数，把我们刚刚的修改生效
    // 将SIGCHLD信号的处理方式修改为我们刚刚自定义的sigchldAction
    // NULL表明忽略信号的上一个动作
    sigaction( SIGCHLD, &sigchldAction, NULL);

    // 类似于上，把stop也进行设置
    // 首先将sigtstpAction变量初始化一下
    memset( &sigtstpAction, 0, sizeof(sigtstpAction));
    // 将函数赋给变量sigtstpAction
    sigtstpAction.sa_handler = Sigtstp;
    // 重置sa_mask
    sigemptyset( &sigtstpAction.sa_mask );
    // 设置sigtstpAction的sa_flags使得这个信号中断后可以自动重启动
    sigtstpAction.sa_flags = SA_RESTART;
    // 将Sigtstp函数赋给SIGTSTP和SIGSTOP信号作为处理函数
    signal(SIGTSTP, Sigtstp);
    //sigaction( SIGTSTP, &sigtstpAction, NULL);
    //sigaction( SIGSTOP, &sigtstpAction, NULL);
}
// Sigchld:子进程结束,前台进程的终止, ctrl+c
void Sigchld( int signo, siginfo_t *info, void *context ){
    // 取出返回信号进程的pid用于查找
    pid_t pid = info->si_pid;
    // 查找在job List中的进程
    int index;
    for( index = 0; index < *jobCount; index ++){
        if( pid == jobList[index].pid ) break;
    }
    // 判断是否找到相应pid的任务
    if( index == *jobCount ){
        // 未找到
        return;
    }
    // 找到了
    else{
        // 根据找到的进程类型进行不同的处理
        if( jobList[index].status == RUN )
            jobList[index].status = DONE;
        else{
            // 啥也不做
        }
    }
}
// Sigtstp:stop当前进程,可以将一个正在前台执行的命令放到后台,并且处于暂停状态 ctrl+z 
void Sigtstp( int signo ){
    printf("\n");
    // 取出返回信号进程的pid用于查找

    int index;
    // 找到最近的前台进程
    for( index = *jobCount - 1; index > 0; index --){
        if( jobList[index].type == FG )  break;
    }
    // 判断是否找到,找到了就挂起，没找到就结束函数
    if( index > 0 ){
        // 在jobList全局变量中改变
        jobList[index].status = SUSPEND;
        jobList[index].type = BG;
        // 用kill函数向该进程发送信号
        if( kill( jobList[index].pid, SIGSTOP ) == -1){
            printf("Failure");
        }
    }
    return ;
}

void PrintBGStatus(pid_t pid){
    int bgCount = 0;
    for(int i = 0; i < *jobCount; ++ i){
        if(jobList[i].type == BG)
            bgCount ++;
    }
    printf("[%d] %d\n", bgCount + 1, pid );
}