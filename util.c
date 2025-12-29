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
        buf[(*pos)++] = ',';
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
    for(int i=0;i<size;i++)
        free(result[i]);
    free(result);
    return buffer;
}



int contains_digit(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (isdigit((unsigned char)s[i]))
            return 1;
    }
    return 0;
}


long long *read_matrix(char *fileName, int N) {
    FILE *f = fopen(fileName, "r");
    if (!f){
        exit(-1);
    }

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

void write_matrix( char *filename, long long *m, int N) {
    FILE *f = fopen(filename, "w");
    if (!f) exit(-1);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(f, "%lld", m[i * N + j]);
            if (j < N - 1) fprintf(f, " ");
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


MatrixTask* mt_create(int job_id, int N, int expected_parts, const char *outname){
    for (int i = 0; i < MAX_PENDING_MJOBS; i++) {
        if (!mtasks[i].in_use) {
            mtasks[i].in_use = 1;
            mtasks[i].job_id = job_id;
            mtasks[i].N = N;
            mtasks[i].expected_parts = expected_parts;
            mtasks[i].received_parts = 0;
            mtasks[i].C =calloc(N *N,sizeof(long long));
            strncpy(mtasks[i].outname, outname, sizeof(mtasks[i].outname) - 1);
            mtasks[i].outname[sizeof(mtasks[i].outname) - 1] =0;
            return &mtasks[i];
        }
    }
    return NULL;
}

MatrixTask* mt_find(int job_id) {
    for (int i = 0; i < MAX_PENDING_MJOBS; i++) {
        if (mtasks[i].in_use && mtasks[i].job_id == job_id) return &mtasks[i];
    }
    return NULL;
}

void mt_finish(MatrixTask *mt) {
    if (!mt) return;
    write_matrix(mt->outname, mt->C, mt->N);
    free(mt->C);
    mt->C = NULL;
    mt->in_use = 0;
}