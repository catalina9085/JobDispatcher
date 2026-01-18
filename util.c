#include "util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


int is_prime(long long n) {
    if (n < 2)
        return 0;

    for (long long i = 2; i * i <= n; i++) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

long long primes(long long n){
    long long cnt=0;
    for(long long i=1;i<=n;i++){
        if(is_prime(i)) cnt++;
    }
    return cnt;
}

long long primeDivisors(long long n){
    long long cnt=0;
    for (long long i = 2; i<= n; i++) {
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


void generateAnagrams(char *str, int l, int r, char *res, int *idx) {
    if (l == r) {
        int len = strlen(str);
        memcpy(res + *idx, str, len);
        *idx += len;
        res[(*idx)++] = '\n';
        return;
    }

    for (int i = l; i <= r; i++) {
        char tmp = str[l];
        str[l] = str[i];
        str[i] = tmp;

        generateAnagrams(str, l + 1, r, res, idx);

        tmp = str[l];
        str[l] = str[i];
        str[i] = tmp;
    }
}


char *anagrams(char *name) {
    int n = strlen(name);
    int count = factorial(n);

    int size = count * (n + 1) + 1;
    char *res = malloc(size);
    if(!res) exit(-1);
    int idx = 0;

    char *temp = strdup(name);
    generateAnagrams(temp, 0, n - 1, res, &idx);
    free(temp);

    res[idx] = '\0';
    return res;
}




int contains_digit(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (isdigit((unsigned char)s[i]))
            return 1;
    }
    return 0;
}


long long *readMatrix(char *fileName, int N) {
    FILE *f = fopen(fileName, "r");
    if (!f) exit(-1);

    long long *m =malloc(N*N*sizeof(long long));
    if (!m) {
        fclose(f);
        exit(-1);
    }

    for (int i = 0; i < N * N; i++) {
        if (fscanf(f, "%lld", &m[i]) != 1) {
            free(m);
            fclose(f);
            exit(-1);
        }
    }

    fclose(f);
    return m;
}

void writeMatrix( char *filename, long long *m, int N) {
    FILE *f = fopen(filename, "w");
    if (!f) exit(-1);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(f, "%lld ", m[i * N + j]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}


 void add(long long *C, long long *A, long long *B, int local_n, int N) {
    int size = local_n * N;
    for (int i = 0; i < size; i++) 
        C[i] = A[i] + B[i];
}

 void mult(long long *C, long long *A, long long *B, int local_n, int N) {
    for (int i = 0; i < local_n; i++) {
        for (int j = 0; j < N; j++) {
            long long sum = 0;
            for (int k = 0; k < N; k++) {
                sum += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}


MatrixTask* create(int job_id, int N, int expected_parts, char *outname){
    for (int i = 0; i < MAX_PENDING_MJOBS; i++) {
        if (!mtasks[i].in_use) {
            mtasks[i].in_use = 1;
            mtasks[i].job_id = job_id;
            mtasks[i].N = N;
            mtasks[i].expected_parts = expected_parts;
            mtasks[i].received_parts = 0;
            mtasks[i].C =calloc(N *N,sizeof(long long));
            if(!mtasks[i].C) exit(-1);
            strncpy(mtasks[i].outname, outname, sizeof(mtasks[i].outname) - 1);
            mtasks[i].outname[sizeof(mtasks[i].outname) - 1] =0;
            return &mtasks[i];
        }
    }
    return NULL;
}

MatrixTask* find(int job_id) {
    for (int i = 0; i < MAX_PENDING_MJOBS; i++) {
        if (mtasks[i].in_use && mtasks[i].job_id == job_id) return &mtasks[i];
    }
    return NULL;
}

void finish(MatrixTask *mt) {
    if (!mt) return;
    writeMatrix(mt->outname, mt->C, mt->N);
    free(mt->C);
    mt->C = NULL;
    mt->in_use = 0;
}