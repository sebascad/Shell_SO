/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell
        (then type ^D to exit program)

**/

#include <string.h>

#include "job_control.h"  // remember to compile with module job_control.c

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void) {
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE / 2];   /* command line (of 256) has max of 128 arguments */

    int pid_fork, pid_wait;   /* pid for created and waited process */
    int status;               /* status returned by wait */
    char *file_in, *file_out; /* file names for redirection */

    while (1) /* Program terminates normally inside get_command() after ^D is typed */
    {
        ignore_terminal_signals();
        printf("COMMAND->");
        fflush(stdout);
        get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */

        if (args[0] == NULL) continue;  // if empty command

        if (strcmp(args[0], "cd") == 0) {
            chdir(args[1]);
            continue;
        }

        pid_fork = fork();

        if (pid_fork == 0) {
            setpgid(getpid(), getpid());

            if (background == 0) {
                tcsetpgrp(STDIN_FILENO, getpid());
            }
            restore_terminal_signals();
            execvp(args[0], args);

            // Si execvp falla
            printf("Error, command not found: %s\n", args[0]);
            exit(-1);

        } else {
            // Proceso padre
            tcsetpgrp(STDIN_FILENO, getpid());
            if (background == 0) {
                // Foreground: esperar
                pid_wait = waitpid(pid_fork, &status, WUNTRACED);

                if (WIFEXITED(status)) {
                    printf("Foreground pid: %d, command: %s, Exited, info: %d\n", pid_wait, args[0],
                           WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("Foreground pid: %d, command: %s, Signaled, info: %d\n", pid_wait,
                           args[0], WTERMSIG(status));
                } else if (WIFSTOPPED(status)) {
                    printf("Foreground pid: %d, command: %s, Suspended, info: %d\n", pid_wait,
                           args[0], WSTOPSIG(status));
                }

                tcsetpgrp(STDIN_FILENO, getpid());  // Make sure shell regains terminal
                ignore_terminal_signals();
            } else {
                // Background: no esperar
                printf("Background job running ... pid: %d, command: %s\n", pid_fork, args[0]);
            }
        }
    }  // end while
}
