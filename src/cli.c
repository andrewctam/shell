#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "cli.h"

const int INIT_NUM_ARGS = 100;

static void run_cmd(char **args, int stdin_fd, int stdout_fd) {
    if (!args) {
        fprintf(stderr, "args is NULL\n");
        abort();
    };

    int pid = fork();
    if (pid == 0) { // child
        if (stdin_fd != STDIN_FILENO) {
            dup2(stdin_fd, STDIN_FILENO);
            close(stdin_fd);
        }
        if (stdout_fd != STDOUT_FILENO) {
            dup2(stdout_fd, STDOUT_FILENO);
            close(stdout_fd);
        }
        
        execvp(args[0], args);

        perror("Failed to start");
        abort();
    } else {
        if (stdin_fd != STDIN_FILENO) {
            close(stdin_fd);
        }
        if (stdout_fd != STDOUT_FILENO) {
            close(stdout_fd);
        }
        int status;
        wait(&status);
    }
}

int run_cli() {
    FILE *in = fdopen(STDIN_FILENO, "r");

    while (1) {
        printf("shell> ");
        fflush(stdout);

        // read a line of input
        char *input = NULL;
        size_t len = 0;
        if (getline(&input, &len, in) == -1) {
            return 1;
        }

        // pipe read fd
        int pipe_rd = -1;

        // parse the input
        const char* SEP = " \n";
        char *cmd = strtok(input, SEP);

        while (cmd) {
            if (strcmp(cmd, "exit") == 0) {
                return 0;
            }
        
            // fds for redirection
            int stdin_fd = STDIN_FILENO;
            int stdout_fd = STDOUT_FILENO;

            // if the previous executable is piping output
            if (pipe_rd != -1) {
                stdin_fd = pipe_rd;
                pipe_rd = -1;
            }

            // flag to tell if we are done parsing args
            int args_ended = 0;
            
            // args array
            char **args = malloc(INIT_NUM_ARGS * sizeof(char *));

            args[0] = cmd;
            args[1] = NULL;
            int num_args = 1;

            // need the last arg to be a NULL
            int cur_max_args = INIT_NUM_ARGS - 1;

            // keep checking for args
            char *next_token = strtok(NULL, SEP);
            while (next_token) {
                if (strcmp(next_token, "&") == 0) {
                    break;
                } else if (strcmp(next_token, ">") == 0 || strcmp(next_token, "<") == 0) {
                    args_ended = 1;
                    // get the next token which should be the file to redirect
                    char *file = strtok(NULL, SEP);
                    int fd = open(file, O_WRONLY | O_CREAT, 0777);
                    if (fd == -1) {
                        fprintf(stderr, "File %s not found\n", file);
                        return -1;
                    }

                    if (*next_token == '>') {
                        stdout_fd = fd;
                    } else {
                        stdin_fd = fd;
                    }

                } else if (strcmp(next_token, "|") == 0) {                    
                    int fds[2];
                    if (pipe(fds)) {
                        fprintf(stderr, "Pipe error\n");
                        abort();
                    }

                    // next executable will read
                    pipe_rd = fds[0];

                    // this executable will write
                    stdout_fd = fds[1];

                    break;
                } else { // an arg
                    if (args_ended) {
                        fprintf(stderr, "Syntax error: arg %s appeared after > or < or |\n", next_token);
                        return -1;
                    }

                    // expand if args array is full
                    if (num_args >= cur_max_args) {
                        cur_max_args += INIT_NUM_ARGS;
                        args = realloc(args, cur_max_args * sizeof(char*));
                    }

                    // set last arg to NULL
                    args[num_args + 1] = NULL;
                    args[num_args++] = next_token;
                }

                next_token = strtok(NULL, SEP);
            }

            run_cmd(args, stdin_fd, stdout_fd);

            cmd = strtok(NULL, SEP);
        }
    }


    return 0;
}
