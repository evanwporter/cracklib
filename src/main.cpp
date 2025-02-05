#include "../include/cracklib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_args(int argc, char** argv, char* file, int* threads, int* json_output) {
    if (argc < 2) {
        printf("USAGE: rarcrack encrypted_archive.ext [--threads NUM] [--json]\n");
        exit(1);
    }

    strcpy(file, argv[1]);
    *threads = 2;
    *json_output = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--threads") == 0) {
            if ((i + 1) < argc) {
                *threads = atoi(argv[++i]);
                if (*threads < 1) *threads = 1;
                if (*threads > 12) *threads = 12;
            }
            else {
                printf("ERROR: Missing parameter for --threads!\n");
                exit(1);
            }
        }
        else if (strcmp(argv[i], "--json") == 0) {
            *json_output = 1;
        }
    }
}

int main(int argc, char* argv[]) {
    char file[255];
    int threads;
    int json_output;

    printf("RarCrack! Modified by Evan William Porter\n\n");

    parse_args(argc, argv, file, &threads, &json_output);

    if (json_output) {
        status_to_json(file);
    }
    else {
        crack(file, threads);
    }

    return EXIT_SUCCESS;
}
