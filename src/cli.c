#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cli.h"

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

        //parse the input
        const char* SEP = " \0\n";
        char *token = strtok(input, SEP);

        while (token) {
            printf("%s\n", token);

            token = strtok(NULL, SEP);
        }
    }


    return 0;
}
