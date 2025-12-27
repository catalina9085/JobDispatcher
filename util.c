#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <mpi.h>

int is_prime(int n) {
    if (n < 2)
        return 0;

    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

int primes(int n){
    int cnt=0;
    for(int i=1;i<=n;i++){
        if(is_prime(i)) cnt++;
    }
    return cnt;
}

int primeDivisors(int n){
    int cnt=0;
    for (int i = 2; i<= n; i++) {
        if (n % i == 0 && is_prime(i))
            cnt++;
    }
    return cnt;
}

int factorial(int n) {
    int f = 1;
    for (int i = 1; i <= n; i++)
        f *= i;
    return f;
}


void swap(char *a, char *b) {
    char tmp = *a;
    *a = *b;
    *b = tmp;
}

void generate(char *str, int l, int r,
              char **result, int *idx) {

    if (l == r) {
        result[*idx] = strdup(str);
        (*idx)++;
        return;
    }

    for (int i = l; i <= r; i++) {
        swap(&str[l], &str[i]);
        generate(str, l + 1, r, result, idx);
        swap(&str[l], &str[i]);
    }
}


void generate_anagrams_buf(char *str, int l, int r,
                           char *buf, int *pos) {
    if (l == r) {
        int len = strlen(str);
        memcpy(buf + *pos, str, len);
        *pos += len;
        buf[(*pos)++] = '\n';
        return;
    }

    for (int i = l; i <= r; i++) {
        char tmp = str[l];
        str[l] = str[i];
        str[i] = tmp;

        generate_anagrams_buf(str, l + 1, r, buf, pos);

        tmp = str[l];
        str[l] = str[i];
        str[i] = tmp;
    }
}


char *anagrams(char *name){
    int n = strlen(name);
    int size = factorial(n);

    char **result = malloc((size) * sizeof(char *));
    int idx = 0;

    char *temp = strdup(name);
    generate(temp, 0, n - 1, result, &idx);
    free(temp);

    int buf_size = size * (n + 1) +1;
    char *buffer = malloc(buf_size);
    idx = 0;
    temp=strdup(name);
    generate_anagrams_buf(temp, 0, n - 1, buffer, &idx);
    if (idx < buf_size) buffer[idx] =0;
    return buffer;
}



int contains_digit(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (isdigit((unsigned char)s[i]))
            return 1;
    }
    return 0;
}

void write_to_file(Result res){
    char filename[64];
    sprintf(filename, "CLI%d.txt", res.client_id);

    FILE *out = fopen(filename, "a");
    if(!out){
        printf("Cannot open file\n");
        fflush(stdout);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if(res.result!=-1){
        fprintf(out,"%d\n",res.result);
    }
    else{
        fprintf(out,"%s\n",res.buffer);
    }
}