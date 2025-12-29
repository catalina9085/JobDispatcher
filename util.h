#ifndef JOB_H
#define JOB_H


#define TAG_JOB     1
#define TAG_RESULT  2
#define TAG_STOP    3

#define TAG_MJOB        4   // joburi cu matrice
#define TAG_MRESULT     5

#define MATRIX_THRESHOLD 64    
#define MAX_PENDING_MJOBS 128 

#define MAX (40320 * 9)  // 8!*(8+'\n')

typedef enum {
    TASK_PRIMES = 0,
    TASK_PRIMEDIVISORS = 1,
    TASK_ANAGRAMS = 2,
    TASK_MATRIXADD =3,
    TASK_MATRIXMULT=4
} TaskType;

typedef struct {
    int client_id;
    TaskType task;
    long long value;   //pt primes si primedivisors
    char name[100];    //pt anagrams
} Job;

typedef struct {
    int client_id;
    long long result;
    char buffer[MAX];  //pt anagrams
    int worker;
} Result;

typedef struct {
    int task;        
    int job_id;
    int N;
    int start_row;
    int local_n;
} MatrixJob;

typedef struct {
    int job_id;
    int worker;
    int N;
    int start_row;
    int local_n;
} MatrixResult;

typedef struct {
    int in_use;
    int job_id;
    int N;
    int expected_parts;
    int received_parts;
    long long *C; //rezultatul
    char outname[256];  //out file
} MatrixTask;

extern MatrixTask mtasks[MAX_PENDING_MJOBS];

long long primes(long long n);
long long primeDivisors(long long n);
char *anagrams(char *name);

int contains_digit(const char *s);

long long *read_matrix(char *fileName, int N);
void write_matrix(char *fileName,long long *m, int N);

 void add(long long *C, long long *A, long long *B, int local_n, int N);
 void mult(long long *C, long long *A, long long *B, int local_n, int N);
MatrixTask* mt_create(int job_id, int N, int expected_parts, const char *outname);
MatrixTask* mt_find(int job_id);
void mt_finish(MatrixTask *mt);

#endif