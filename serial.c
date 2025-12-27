/*
gcc -Wall serial.c util.c -o serial.exe
./serial.exe
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "util.h"

void write_to_file(Result res){
    char filename[64];
    sprintf(filename, "CLI%d_serial.txt", res.client_id);

    FILE *out = fopen(filename, "a");
    if(!out){
        printf("Cannot open file\n");
        fflush(stdout);
        exit(-1);
    }

    if(res.result!=-1){
        fprintf(out,"%lld\n",res.result);
    }
    else{
        fprintf(out,"%s\n",res.buffer);
    }
}

int main(int argc, char *argv[]) {
    FILE *f = fopen("commands.txt", "r");
    if (!f) {
        printf("Cannot open file\n");
        fflush(stdout);
        exit(-1);
    }
    int eof = 0;
    char line[256];
    struct timespec start, finish;
    double elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (!eof) {
        if (fgets(line, sizeof(line), f)) {
            char cli[32];
            char cmd[32];
            char arg[100];
            Result res;
            sscanf(line, "%s %s %s", cli, cmd, arg);
            sscanf(cli, "CLI%d", &res.client_id);

            if (strcmp(cmd, "PRIMES") == 0) {
                    long long value = atoi(arg);
                    res.result=primes(value);

            }
            else if (strcmp(cmd, "PRIMEDIVISORS") == 0) {
                long long value = atoi(arg);
                res.result=primeDivisors(value);
            }
            else if (strcmp(cmd, "ANAGRAMS") == 0) {
                char *result=anagrams(arg);
                strcpy(res.buffer,result);
                res.result=-1;
                free(result);
            }
            else {
                printf("Unknown command: %s\n", cmd);
                continue;
            }
            write_to_file(res);
        } else {
            eof = 1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Serial execution time: %.3f seconds\n",elapsed);
    return 0;
}


/*
Results:
3.232 sec
*/