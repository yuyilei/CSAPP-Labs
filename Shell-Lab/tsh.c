/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */   // 给某些信号注册信号处理函数....
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    }
    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
*/
void eval(char *cmdline)
{
    int olderrno = errno ;
    char *argv[MAXARGS + 1] ;
    int bg = parseline(cmdline, argv) ;
    if ( argv[0] == NULL ) {
        return ;   //  没有命令直接返回
    }
    if( !builtin_cmd(argv) ) {
        pid_t pid ;
        sigset_t now ;
        sigemptyset( &now ) ;    //清空信号set
        sigaddset( &now , SIGCHLD ) ;  // 加入SIGCHID 信号
        sigprocmask( SIG_BLOCK, &now, NULL ) ; // 阻塞父进程的SIGCHILD信号 。。。。
        pid = fork() ;
        if( pid  < 0)  {  //出错
            unix_error("Fork Error!") ;
        }
        else if ( !pid ) {    //child
            sigprocmask(SIG_SETMASK, &now, NULL) ; 　// 子进程继承父进程的阻塞集合，所以要取消阻塞，这样对不。。。。
            setpgid( 0 , 0 ) ; // 创建新的进程组，将当前进程加入
            if (execve(argv[0], argv, environ) < 0) {  // 有无此命令,若有，加载并运行程序
                printf("%s: Command not found\n", argv[0]) ;
                exit(0) ;
                }
            }
        else {
          //  if (execve(argv[0], argv, environ) < 0) {  return ; }  // 有无此命令,若有，加载并运行程序
            addjob(jobs, pid, ((bg == 1) ? BG : FG), cmdline) ;  // 是否在后台执行
            sigprocmask(SIG_SETMASK, &now, NULL) ;      //  取消父进程中的阻塞
            if ( !bg ) {
                    waitfg(pid) ; // 暂时停止目前进程，直到子进程结束
            }
            else  {
                printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline) ; // jid , pid , command line
            }
        }
    }
    errno = olderrno ;
    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv) // 分离命令行，判断fg和bg
{
    int olderrno = errno ;
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }
    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;

    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if (( bg = (*argv[argc-1] ) == '&' )) {
	argv[--argc] = NULL;
    }
    errno = olderrno ;
    return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)   // quit , jobs , bg , fg , &
{
    if ( !strcmp(argv[0] , "quit" ) ){
        exit(0) ;
    }
 	if ( !strcmp(argv[0], "jobs") ) {
        listjobs(jobs) ;
        return 1 ;
    }
    if ( !strncmp(argv[0], "bg", 2) || !strncmp(argv[0], "fg", 2) ) {
        do_bgfg(argv);
        return 1;
    }
    return 0 ;    /* not a builtin command */
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv)
{
    char *ID = argv[1] ;
    if ( ID == NULL ) {
        printf("%s command requires PID or JID  argument\n", argv[0]) ;
        return ;
    }
    int jid ;
    struct job_t *job ;
    pid_t pid ;
	if( ID[0] == '%') {
        jid = atoi(&ID[1]) ;
        job = getjobjid(jobs, jid) ;
        if(job == NULL){
            printf("ERROR No such job %s\n", ID ) ;
            return ;
        }
        else{
            pid = job->pid;
        }
    }
	else {
        pid = atoi(ID) ;
        job = getjobpid(jobs, pid) ;
        if(job == NULL){
            printf("(%d): No such process\n", pid) ;
            return ;
        }
    }
	kill(-pid, SIGCONT) ;
    if(!strcmp("fg", argv[0])) {
        waitfg(job->pid) ;
    }
    else{
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
    job->state = !strcmp("fg", argv[0]) ? FG : BG ;
    return ;
}


/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    struct job_t* job ;
    job = getjobpid(jobs,pid) ;
    if(pid == 0){
        return ;
    }
    if(job != NULL){
        while(pid==fgpid(jobs)) { ; }
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig)   // 当子进程结束或变为僵死进程时，向父进程发送信号，回收僵死进程
{
    int olderrno = errno ;
 	int status ;
    pid_t pid ;
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {  // 对于每个前台的进程，如果没有任何子进程停止或终止，立即返回
        if (WIFSTOPPED(status)){  // 引起返回的子进程当前是被停止的
            getjobpid(jobs, pid)->state = ST ;
            int jid = pid2jid(pid) ;
            printf("Job [%d] (%d) Stopped by signal %d\n", jid, pid, WSTOPSIG(status));
        }
        else if (WIFSIGNALED(status)){  // 子进程因为未被捕获的信号终止
            int jid = pid2jid(pid) ;
            printf("Job [%d] (%d) Stopped by unkown signal \n", jid, pid );
            deletejob(jobs, pid) ;
        }
        else if (WIFEXITED(status)){ // 子进程调用exit或return正常终止，返回子进程的推出状态
            deletejob(jobs, pid);
            printf("1\n") ;
        }
    }
    errno = olderrno ;
    return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)          // ctrl-c 停止前台所有进程，一个进程组里只有一个进程
{
    int olderrno = errno ;
    pid_t pid = fgpid(jobs) ;
    if ( pid ) {
        kill(-pid,pid) ; // 整个进程组发SIGINT
        deletejob(jobs,pid) ;
    }
    errno = olderrno ;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)        // ctrl-z 挂起
{
    int olderrno = errno ;
    pid_t pid = fgpid(jobs) ;
    if ( pid ) {
        kill(-pid,sig) ; // 整个进程组发
    }
    errno = olderrno ;
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {   // 清除所有job
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) { // 初始化jobs
    int i;
    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)      // 找出最大的job ID
{
    int i, max=0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)  // 把job添加入jobs
{
    int i;
    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
        }
            return 1;
	    }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)    // 删除某个进程的job
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {     // 返回最早的前台的job的进程ID
    for ( int i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) { // 找出某一个进程的最早的job ， 返回只想结构体的指针
    if (pid < 1)
	return NULL;
    for (int i  = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    if (jid < 1)
	return NULL;
    for ( int i = 0; i < MAXJOBS; i++) // 在jobs 里找出某一个job
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)  // 找出某一个进程的job ，返回jid
{
    if (pid < 1)
	return 0;
    for ( int i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)   // 列出jobs
{
    for ( int i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG:
		    printf("Running ");
		    break;
		case FG:
		    printf("Foreground ");
		    break;
		case ST:
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ",
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno)); //   全局变量errorn表示出了什么错，调用系统函数时
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg); // 调用普通函数
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) // 就是sigaction函数的装饰器咯。。。。
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  // 只有这个处理程序正在处理的那种类型的信号被阻塞。。。怎么做到的。。？？
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */ // 初始化信号集合为空
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */
    // sigaction 信号安装处理函数， 将action设置为信号signum的处理函数，将旧的信号处理函数储存到old_action里
    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);  // 为什么返回old_action ？？
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



