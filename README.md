# simpleShell_&_scheduler
Team Member:- Maulik Mahey(2022282) Mayank Kumar(2022284)

Contribution:- Maulik Mahey-I did some  parts of the simple Shell and implemented error handling in all possible places. Mayank Kumar-He has  done the simpleScheduler.

# SimpleScheduler:

Purpose: SimpleScheduler is a daemon responsible for CPU scheduling activities in an operating system context.

Scheduling Algorithm: It implements a round-robin scheduling algorithm for a specified number of CPU cores (NCPU) and time quantum (TSLICE). Processes are added to a ready queue and scheduled for execution.

Communication with Processes: It signals processes to start execution and manages their execution for the given time quantum.

Termination: SimpleScheduler continues to work until all user-submitted jobs have naturally terminated.

Priority Scheduling: Optionally, it supports priority scheduling, allowing users to assign priorities to their jobs.

# SimpleShell:

Purpose: SimpleShell is a command-line interface that allows users to submit and manage executable programs for execution.

User Input: It takes user input to submit executable programs (e.g., "./a.out") and, optionally, set their priority (e.g., "./a.out 2").

Process Management: It creates new processes to run the submitted executables, sends these processes to SimpleScheduler, and handles process termination.
