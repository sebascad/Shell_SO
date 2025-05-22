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
job* jobList;

void manejador(int sig) {
    int pid;
    int status;
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG | WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            printf("Foreground pid: %d, command: %s, Exited, info: %d\n", pid,
                   get_item_bypid(jobList, pid)->command, WEXITSTATUS(status));
            delete_job(jobList, get_item_bypid(jobList, pid));
        } else if (WIFSIGNALED(status)) {
            job* j = get_item_bypid(jobList, pid);
            printf("Foreground pid: %d, command: %s, Signaled\n", pid, j->command,
                   WIFSIGNALED(status));

            delete_job(jobList, j);
        } else if (WIFSTOPPED(status)) {
            get_item_bypid(jobList, pid)->state = STOPPED;
        } else if (WIFCONTINUED(status)) {
            job* j = get_item_bypid(jobList, pid);
            printf("Background process %s (%d) continued", j->command, j->pgid);
            j->state = BACKGROUND;
        }
    }
}

void fg(const char* n) {
    int num;
    if (n == NULL) {
        num = 1;
    } else {
        num = atoi(n);
    }

    block_SIGCHLD();
    job* aux = get_item_bypos(jobList, num);
    unblock_SIGCHLD();

    int status;
    if (aux == NULL) exit(-1);
    int pid = aux->pgid;
    char* command = aux->command;

    tcsetpgrp(STDIN_FILENO, pid);

    aux->state = FOREGROUND;

    killpg(aux->pgid, SIGCONT);

    waitpid(pid, &status, WUNTRACED);

    tcsetpgrp(STDIN_FILENO, getpid());

    if (WIFEXITED(status)) {
        printf("Foreground pid: %d, command: %s, Exited, info: %d\n", pid, command,
               WEXITSTATUS(status));

        block_SIGCHLD();
        delete_job(jobList, aux);
        unblock_SIGCHLD();
    } else if (WIFSTOPPED(status)) {
        printf("Foreground pid: %d, command: %s, Suspended, info: %d\n", pid, command,
               WSTOPSIG(status));

        block_SIGCHLD();
        aux->state = STOPPED;
        unblock_SIGCHLD();
    } else if (WIFSIGNALED(status)) {
        printf("Foreground pid: %d, command: %s, Signaled, info: %d\n", pid, command,
               WIFSIGNALED(status));

        block_SIGCHLD();
        delete_job(jobList, aux);
        unblock_SIGCHLD();
    }
}

void bg(const char* n) {
    int num;
    if (n == NULL) {
        num = 1;
    } else {
        num = atoi(n);
    }

    block_SIGCHLD();
    job* aux = get_item_bypos(jobList, num);
    if (aux == NULL) {
        unblock_SIGCHLD();
        exit(-1);
    }

    aux->state = BACKGROUND;
    unblock_SIGCHLD();
    killpg(aux->pgid, SIGCONT);

    printf("Background job running ,pid: %d, command: %s\n", aux->pgid, aux->command);
}

// -----------------------------------------------------------------------
//                            MAIN
// -----------------------------------------------------------------------

int main(void) {
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char* args[MAX_LINE / 2];   /* command line (of 256) has max of 128 arguments */

    int pid_fork, pid_wait;   /* pid for created and waited process */
    int status;               /* status returned by wait */
    char *file_in, *file_out; /* file names for redirection */
    jobList = new_list("Lista jobList");

    while (1) /* Program terminates normally inside get_command() after ^D is typed */
    {
        printf("COMMAND->");
        fflush(stdout);
        ignore_terminal_signals();  // El shell ignora las señales del terminal

        signal(SIGCHLD, manejador);

        get_command(inputBuffer, MAX_LINE, args, &background); /* get next command */

        parse_redirections(args, &file_in, &file_out);

        if (args[0] == NULL) continue;  // if empty command

        // Comandos internos
        if (strcmp(args[0], "cd") == 0) {
            chdir(args[1]);
            continue;
        }
        if (strcmp(args[0], "jobs") == 0) {
            if (empty_list(jobList)) printf("Lista de trabajos vacia. \n");
            print_job_list(jobList);
            continue;
        }
        if (strcmp(args[0], "fg") == 0) {
            fg(args[1]);
            continue;
        }
        if (strcmp(args[0], "bg") == 0) {
            bg(args[1]);
            continue;
        }

        pid_fork = fork();  // Creamos el proceso hijo

        if (pid_fork == 0) {
            // Agrupamos los procesos hijos
            setpgid(getpid(), getpid());

            // Si no esta en segundo plano
            if (background == 0) {
                tcsetpgrp(STDIN_FILENO, getpid());  // Otorga permisos de terminal al grupo
            }

            // Los procesos hijos deben de poder recibir señales del terminal
            restore_terminal_signals();

            
            
            
            execvp(args[0], args);  // Ejecuta el comando

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
                    block_SIGCHLD();
                    add_job(jobList, new_job(pid_fork, args[0], STOPPED));
                    unblock_SIGCHLD();
                }

                tcsetpgrp(STDIN_FILENO, getpid());  // Shell recupera el termianl
                ignore_terminal_signals();
                
            } else {
                // Background: no esperar
                printf("Background job running ... pid: %d, command: %s\n", pid_fork, args[0]);
                block_SIGCHLD();
                add_job(jobList, new_job(pid_fork, args[0], BACKGROUND));
                unblock_SIGCHLD();
            }
        }
    }  // end while
}
