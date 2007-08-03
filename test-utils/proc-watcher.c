/*
 * Process watcher.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

int timeout; // seconds
pid_t child_pid;

void
alarm_handler(int sig)
{
    (void)sig;
    fprintf(stderr, "Timeout waiting for child....\n");
    kill(child_pid, SIGINT);
    usleep(1000000);
    kill(child_pid, SIGTERM);
    exit(1);
}

int
main(int argc, char* argv[])
{
    char* child_program;
    char* child_argv[256];
    int i, err;
    int child_stdout[2];

    if (argc < 3) {
	printf("Usage: %s <timeout> command...\n\n", argv[0]);
	printf("Process Watcher - Execs a process and watches the stdout\n"
	       "of the program. If there is a period of inactivity, the\n"
	       "watcher will kill the child and itself and return an error.\n");
	printf("\n");
	exit(1);
    }

    argc--; argv++;
    timeout = atoi(argv[0]);
    argc--; argv++;
    child_program = argv[0];
    argc--; argv++;

    if (argc > 254) {
	fprintf(stderr, "Warning: too many arguments, truncating...\n");
	argc = 254;
    }

    child_argv[0] = child_program;
    for (i = 0; i<argc; ++i) {
	child_argv[i + 1] = argv[i];
    }
    child_argv[i + 1] = 0;

    pipe(child_stdout);
    
    child_pid = fork();
    if (child_pid == 0)
    {
        // Attach child stdout to the pipe
        close(1);
        close(child_stdout[0]);
        dup2(child_stdout[1], 1);
        fprintf(stderr, "%s %s %s\n",  
                child_program, child_argv[0], child_argv[1]);
        err = execv(child_program, child_argv);
        if (err != 0) {
            fprintf(stderr, "Couldn't execute %s\n", child_program);
            exit(1);
        }
    }
    else
    {
        char readbuf[1024];
        int cc;

        signal(SIGALRM, alarm_handler);

        // read from the child, setting a timeout
        alarm(timeout);
        
        for (;;)
        {
            cc = read(child_stdout[0], readbuf, 1024);
            alarm(timeout);
            if (cc == -1)
            {
                switch (errno) {
                case EINTR:
                    fprintf(stderr, "Program timed out, killing\n");
                break;
                default:
                    fprintf(stderr, "Error in read: %s", strerror(errno));
                    exit(1);
                }
            }
            else if (cc == 0)
            {
                exit(0);
            }
            write(1, readbuf, cc);
        }
    }
    
    return 0;
}
