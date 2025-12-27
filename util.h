#ifndef JOB_H
#define JOB_H


#define TAG_JOB     1
#define TAG_RESULT  2
#define TAG_STOP    3

#define MAX (40320 * 9)  // 8!*(8+'\n')

typedef enum {
    TASK_PRIMES = 0,
    TASK_PRIMEDIVISORS = 1,
    TASK_ANAGRAMS = 2
} TaskType;

typedef struct {
    int client_id;
    TaskType task;
    int value;   //pt primes si primedivisors
    char name[100];    //pt anagrams
} Job;

typedef struct {
    int client_id;
    int result;
    char buffer[MAX];  //pt anagrams
    int worker;
} Result;

int primes(int n);
int primeDivisors(int n);
char *anagrams(char *name);


void write_to_file(Result res);
int contains_digit(const char *s);
#endif