#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h> 

struct Process
{
    int pid;
    int execution_time;
    int wait_time;
};

#define MAX_QUEUE_SIZE 100 // Change this value to the desired maximum queue size

struct Queue
{
    struct Process items[MAX_QUEUE_SIZE];
    int front, rear;
};

// Function to create an empty queue
void initializeQueue(struct Queue *q)
{
    q->front = -1;
    q->rear = -1;
}

// Function to check if the queue is empty
bool isEmpty(struct Queue *q)
{
    return q->front == -1;
}

// Function to check if the queue is full
bool isFull(struct Queue *q)
{
    return (q->rear + 1) % MAX_QUEUE_SIZE == q->front;
}

// Function to enqueue (add) an element to the queue
void enqueue(struct Queue *q, struct Process item)
{
    if (isFull(q))
    {
        printf("Queue is full. Cannot enqueue %d.\n", item);
        return;
    }
    if (isEmpty(q))
    {
        q->front = 0;
        q->rear = 0;
    }
    else
    {
        q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;
    }
    q->items[q->rear] = item;
}

// Function to dequeue (remove) an element from the queue
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

bool containsElement(struct Queue *queue, int pid) {
    if (isEmpty(queue)) {
        return false;
    }

    int i = queue->front;

    while (i <= queue->rear) {
        if (queue->items[i].pid == pid) {
            return true; // Element found in the queue
        }
        i++;
    }

    return false; // Element not found in the queue
}


int main(int argc, char *argv[])
{
    int Scheduler_pid = getpid();
    int fd1 = open("helping_information", O_WRONLY);
    write(fd1, &Scheduler_pid, sizeof(int));
    close(fd1);
    kill(Scheduler_pid, SIGSTOP);
    struct Queue q;
    initializeQueue(&q);
    int fdn = open("helping_information", O_RDONLY);
    int NCPU, time_quantum;
    read(fdn, &NCPU, sizeof(int));
    read(fdn, &time_quantum, sizeof(int));
    close(fdn);
    int fd[2];
    if (pipe(fd)==-1){
        printf("Pipe Failed\n");
        exit(0);
    }
    while (true)
    {
        int process_pid;
        int fd2 = open("process_pids", O_RDONLY);
        if (fd2 == -1)
        {
            printf("Error occured while opening read descriptor in Scheduler\n");
            exit(0);
        }
        if (read(fd2, &process_pid, sizeof(int)) == -1)
        {
            printf("Error occured while opening reading in Scheduler\n");
            exit(0);
        }
        struct Process p1;
        p1.pid = process_pid;
        p1.execution_time = 0;
        p1.wait_time=0;
        enqueue(&q, p1);
        close(fd2);
        close(fd[0]);
        write(fd[1],&p1,sizeof(struct Process));
        close(fd[1]);
        kill(Scheduler_pid, SIGSTOP);
    }

    int pid = fork();
    if (pid == -1)
    {
        perror("Fork failed\n");
    }
    if(pid==0)
    {
        struct Queue Running_queue;
        struct Queue Complete_queue;
        while(!isEmpty(&q)){
            struct Process pro1;
            read(fd[0],&pro1,sizeof(struct Process));
            if(!containsElement(&q,pro1.pid)){
                enqueue(&q,pro1);
            }
            for(int i=0;i<=NCPU;i++){
                struct Process pro;
                pro=dequeue(&q);
                kill(pro.pid,SIGCONT);
                enqueue(&Running_queue,pro);
            }
            usleep(time_quantum*1000);

            for(int i=NCPU+1;i<=q.rear;i++){
                struct Process pro3;
                pro3.wait_time+=time_quantum;
            }
            for(int i=0;i<=NCPU;i++){
                struct Process pro2;
                pro2=dequeue(&Running_queue);
                kill(pro2.pid,SIGSTOP);
                enqueue(&Running_queue,pro2);
            }
            //Update info of processes
            for(int j=0;j<=NCPU;j++){
                int status;
                struct Process process1;
                process1=dequeue(&Running_queue);
                int result =waitpid(process1.pid,&status,0);
                if(WIFEXITED(status)){
                    process1.execution_time+=time_quantum;
                    enqueue(&Complete_queue,process1);
                }else{
                    process1.execution_time+=time_quantum;
                    enqueue(&q,process1);
                }
            }
        }

        int file_descriptor=open("final_info",O_WRONLY);
        if(file_descriptor==-1){
            printf("Error while opening file_descriptor\n");
            exit(0);
        }
        int x=Complete_queue.rear+1;
        if(write(file_descriptor,&x,sizeof(int)==-1)){
            printf("Error while writing size of Complete_queue\n");
            exit(0);
        }

        if(write(file_descriptor,&Complete_queue,sizeof(struct Queue) ==-1)){
            printf("Error while writing final_info\n");
            exit(0);
        }
        close(file_descriptor);
    }else{
        wait(NULL);
    }


}
