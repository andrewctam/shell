#include <stdlib.h>

#include "cli.h"

int main(int argc, char** argv) {
    if (run_cli()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
