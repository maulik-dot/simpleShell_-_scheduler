// C Program to design a shell in Linux
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include<readline/history.h>
#include <sys/time.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<errno.h>
#include<fcntl.h>
#include<signal.h>
#include<stdbool.h>

#define MAXLETTER 1000
#define MAXCOMMANDS 1000
#define MAX_QUEUE_SIZE 100

int NCPU,time_quantum;
int Scheduler_pid;

struct Process
{
    int pid;
    int execution_time;
    int wait_time;
};

struct Queue
{
    struct Process items[MAX_QUEUE_SIZE];
    int front, rear;
};

struct CommandInfo {
    char command[MAXLETTER];
    pid_t pid;
    struct timeval start_time;
    double execution_time;
};

#define MAX_HISTORY_SIZE 100
struct CommandInfo commandHistory[MAX_HISTORY_SIZE];
int historyIndex = 0; 



void clearScreen() {
    printf("\033[H\033[J");
}

void shell(){
    clearScreen();
    printf("WELCOME TO MY SHELL\n");
    printf("\n");
    sleep(2);
}
bool isEmpty(struct Queue *q)
{
    return q->front == -1;
}

struct Process dequeue(struct Queue *q)
{
    if (isEmpty(q))
    {
        printf("Queue is empty. Cannot dequeue.\n");
        exit(0); // You can choose a different value to indicate an error
    }
    struct Process item = q->items[q->front];
    if (q->front == q->rear)
    {
        q->front = -1;
        q->rear = -1;
    }
    else
    {
        q->front = (q->front + 1) % MAX_QUEUE_SIZE;
    }
    return item;
}



int Input(char* str)
{
	char* buf;

	buf = readline("\n>>> ");
	if (strlen(buf) != 0) {
		add_history(buf);
		strcpy(str, buf);
		return 0;
	} else {
		return 1;
	}
}

void runInBackground(char* program) {
    // Fork a child process
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Child process
        // Detach from the shell's controlling terminal
        setsid();
        // Redirect standard input, output, and error to /dev/null
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        // Execute the program
        if (execlp(program, program, NULL) == -1) {
            perror("execlp");
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        kill(pid,SIGSTOP);
        kill(Scheduler_pid,SIGCONT);
        int fd=open("process_pids",O_WRONLY);
        if(write(fd,&pid,sizeof(int))==-1){
            printf("Error while writing!!\n");
            exit(0);
        }
        close(fd);
        wait(NULL);
    }
}

// Function where the system command is executed
void execArgs(char** parsed) {
    // printf("%s",parsed);
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    // Forking a child
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    } else if (pid == 0) {
        // Child process

        if (execvp(parsed[0], parsed) < 0) {
            perror("execvp");
            exit(1); // Exit with a non-zero status to indicate an error
        }


        exit(0);
    } else {
        // Parent waiting for child to terminate
        int status;
        pid_t wpid = waitpid(pid, &status, 0);
        if (wpid == -1) {
            perror("waitpid");
        } else {
            if (WIFEXITED(status)) {
                // printf("Command (PID: %d) exited with status %d\n", pid, WEXITSTATUS(status));
                gettimeofday(&end_time, NULL);
                double execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1e6;
                strncpy(commandHistory[historyIndex].command, parsed[0], MAXLETTER);
                commandHistory[historyIndex].pid = pid;
                commandHistory[historyIndex].start_time=start_time;
                commandHistory[historyIndex].execution_time = execution_time;

                historyIndex++;

            }
        }
    }
}


void execArgsPiped(char** parsed, char** parsedpipe) {
    // 0 is read end, 1 is write end
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        perror("Pipe could not be initialized");
        return;
    }

    struct timeval start_time1, end_time1;
    gettimeofday(&start_time1, NULL);
    p1 = fork();
    if (p1 < 0) {
        perror("Could not fork");
        return;
    }

    if (p1 == 0) {
        // Child 1 executing..
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0) {
            perror("Could not execute command 1");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent executing
        struct timeval start_time2, end_time2;
        gettimeofday(&start_time2, NULL);
        p2 = fork();

        if (p2 < 0) {
            perror("Could not fork");
            return;
        }
        if(p2==0){
        // Child 2 executing..
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            if (execvp(parsedpipe[0], parsedpipe) < 0) {
                perror("Could not execute command 2");
                exit(EXIT_FAILURE);
            }
        } else {
            // Parent
            close(pipefd[0]);  // Close both ends in parent
            close(pipefd[1]);

            int status1, status2;
            waitpid(p1, &status1, 0);
            if (WIFEXITED(status1)) {
                gettimeofday(&end_time1, NULL);
                double execution_time1 = (end_time1.tv_sec - start_time1.tv_sec) + (end_time1.tv_usec - start_time1.tv_usec) / 1e6;
                strncpy(commandHistory[historyIndex].command, parsed[0], MAXLETTER);
                commandHistory[historyIndex].pid = p1;
                commandHistory[historyIndex].start_time = start_time1;
                commandHistory[historyIndex].execution_time = execution_time1;
                historyIndex++;
            }
            waitpid(p2, &status2, 0);
            if (WIFEXITED(status2)) {
                gettimeofday(&end_time2, NULL);
                double execution_time2 = (end_time2.tv_sec - start_time2.tv_sec) + (end_time2.tv_usec - start_time2.tv_usec) / 1e6;
                strncpy(commandHistory[historyIndex].command, (p1 == 0) ? parsed[0] : parsedpipe[0], MAXLETTER);                commandHistory[historyIndex].pid = p2;
                commandHistory[historyIndex].start_time = start_time2;
                commandHistory[historyIndex].execution_time = execution_time2;
                historyIndex++;
            }
        }
    }
}


// Function to execute builtin commands
int ownCmdHandler(char** parsed) {
    int NoOfOwnCmds = 2, i, switchOwnArg = 0;
    char* ListOfOwnCmds[NoOfOwnCmds];
    char* username;

    ListOfOwnCmds[0] = "exit";
    ListOfOwnCmds[1] = "cd";

    for (i = 0; i < NoOfOwnCmds; i++) {
        if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0) {
            switchOwnArg = i + 1;
            break;
        }
    }

    switch (switchOwnArg) {
        case 1:
            printf("\nCommand HISTORY\n");
            for (int i = 0; i < historyIndex; i++) {
                printf("---------------------------\n");
                printf("Command: %s\n", commandHistory[i].command);
                printf("PID: %d\n", commandHistory[i].pid);
                printf("Start Time: %ld.%06ld\n", commandHistory[i].start_time.tv_sec, commandHistory[i].start_time.tv_usec);
                printf("Execution Time: %.6f seconds\n", commandHistory[i].execution_time);
            }

            int file_descriptor=open("final_info",O_RDONLY);
            if(file_descriptor==-1){
                printf("Error while opening file_descriptor\n");
                exit(0);
            }
            int size;
            struct Queue result;
            if(read(file_descriptor,&size,sizeof(int))==-1){
                printf("Error while reading size of Complete_queue\n");
                exit(0);
            }
            if(read(file_descriptor,&result,size)){
                printf("Error while reading size of Complete_queue\n");
                exit(0);
            }
            close(file_descriptor);

            printf("\n\n\nInformation of processes scheduled : \n\n");
            for(int i=0;i<size;i++){
                printf("---------------------------\n");
                struct Process pros;
                pros=dequeue(&result);
                printf("PID : %d\n",pros.pid);
                printf("Execution_time : %d\n",pros.execution_time);
                printf("Wait_time : %d\n",pros.wait_time);
            }

            printf("\nGoodbye\n");
            exit(0);
        case 2:
            if (parsed[1] == NULL) {
                // Change to the initial directory
                chdir(parsed[1]);
            } else {
                // Change to the specified directory
                if (chdir(parsed[1]) != 0) {
                    perror("cd");
                }
            }
            return 1;
        default:
            break;
    }

    return 0;
}

// function for finding pipe
int parsePipe(char* str, char** strpiped)
{

    int pipeCount = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '|') {
            pipeCount++;
        }
    }
    
	int i;
    
	for (i = 0; i < pipeCount+1; i++) {
		strpiped[i] = strsep(&str, "|");
		if (strpiped[i] == NULL)
			break;
	}

	if (pipeCount == 0)
		return 0; // returns zero if no pipe is found.
	else {
		return 1;
	}
}

// function for parsing command words
void parseSpace(char* str, char** parsed)
{
	int i;

	for (i = 0; i < MAXCOMMANDS; i++) {
		parsed[i] = strsep(&str, " ");

		if (parsed[i] == NULL)
			break;
		if (strlen(parsed[i]) == 0)
			i--;
	}
}

int strr(char* str, char** parsed, char** parsedpipe){

	char* strpiped[2];
	int piped = 0;

	piped = parsePipe(str, strpiped);

	if (piped) {
		parseSpace(strpiped[0], parsed);
		parseSpace(strpiped[1], parsedpipe);

	} else {

		parseSpace(str, parsed);
	}

	if (ownCmdHandler(parsed))
		return 0;
	else
		return 1 + piped;
}

int main(){
    char input[MAXLETTER];
    char *parsedArgs[MAXCOMMANDS],*parsedArgsPiped[MAXCOMMANDS];
    char dir[1000];
    int execFlag=0;

    shell();
    printf("Enter NCPU value: ");
    scanf("%d",&NCPU);
    printf("Enter the value of time_quantum(milliseconds) : ");
    scanf("%d",&time_quantum);

    int fd1=open("helping_information",O_RDONLY);
    if(fd1==-1){
        printf("Error while reading helping information\n");
        exit(0);
    }
    read(fd1,&Scheduler_pid,sizeof(int));
    printf("Scheduler_pid %d\n",Scheduler_pid);
    close(fd1);

    int fd5=open("helping_information",O_WRONLY);
    if(fd5==-1){
        printf("Error while writing helping information\n");
        exit(0);
    }
    write(fd5,&NCPU,sizeof(int));
    write(fd5,&time_quantum,sizeof(int));
    close(fd5);

    do {
        getcwd(dir, sizeof(dir));
        printf("\n %s", dir);
        if (Input(input)) {
            continue;
        }

        // Check if the input is the "submit" command
        if (strncmp(input, "submit", 6) == 0) {
            // Extract the program name from the input
            char* program = input + 7;
            // Run the program in the background
            runInBackground(program);

        } else {
            execFlag = strr(input, parsedArgs, parsedArgsPiped);

            if (execFlag == 1)
                execArgs(parsedArgs);

            if (execFlag == 2)
                execArgsPiped(parsedArgs, parsedArgsPiped);
        }
    } while (1);
    
	return 0;
}
