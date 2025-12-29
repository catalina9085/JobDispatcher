/*
gcc -g main.c util.c -I "C:\Program Files (x86)\Microsoft SDKs\MPI\Include" -L "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" -lmsmpi -o main.exe
mpiexec -n 3 main.exe
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

MatrixTask mtasks[MAX_PENDING_MJOBS];


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
        fprintf(out,"%lld\n",res.result);
    }
    else{
        fprintf(out,"%s\n",res.buffer);
    }
    fclose(out);
}

void handle_matrix_command(char *line,int size,int *busy,int *jobs_cnt)
{
    static int next_job_id = 1;

    char cmd[32], f1[128], f2[128];
    int N;

    if (sscanf(line, "%31s %d %127s %127s", cmd, &N, f1, f2) != 4) {
        printf("Invalid command: %s\n", line);
        fflush(stdout);
        return;
    }

    int task = (strcmp(cmd, "MATRIXADD") == 0) ? TASK_MATRIXADD : TASK_MATRIXMULT;

    long long *A = read_matrix(f1, N);
    long long *B = read_matrix(f2, N);
    if (!A || !B) {
        free(A); free(B);
        exit(-1);
    }

    //group ul de threaduri
    int group[1024], group_size = 0;

    if (N > MATRIX_THRESHOLD) {
        for (int i = 1; i < size; i++)
            if (!busy[i]) group[group_size++] = i;
    } else {
        for (int i = 1; i < size; i++)
            if (!busy[i]) { group[group_size++] = i; break; }
    }

    if (group_size == 0) {
        printf("No free workers for matrix operation\n");
        fflush(stdout);
        free(A); free(B);
        return;
    }

    int job_id = next_job_id++;

    char outname[256];
    sprintf(outname, "%s_result_%dx%d_job%d.txt",
        (task == TASK_MATRIXADD ? "ADD" : "MULT"),
        N, N, job_id);

    MatrixTask *mt=mt_create(job_id, N, group_size, outname);
    if(!mt){
        printf("No free MATRIX slots\n");
        fflush(stdout);
        return;
    }


    int rows = N / group_size, rest = N % group_size, start = 0;

    for (int i = 0; i < group_size; i++) {
        int current = group[i];
        int local_n = rows + (i < rest ? 1 : 0);

        MatrixJob mj = { task, job_id, N, start, local_n };

        MPI_Send(&mj, sizeof(mj), MPI_BYTE, current, TAG_MJOB, MPI_COMM_WORLD);
        MPI_Send(A + start * N, local_n * N, MPI_LONG_LONG, current, 0, MPI_COMM_WORLD);

        if (task == TASK_MATRIXADD)
            MPI_Send(B + start * N, local_n * N, MPI_LONG_LONG, current, 0, MPI_COMM_WORLD);
        else
            MPI_Send(B, N * N, MPI_LONG_LONG, current, 0, MPI_COMM_WORLD);

        busy[current] = 1;
        (*jobs_cnt)++;
        start += local_n;
    }

    printf("[MAIN] MATRIX job %d dispatched (%d workers)\n", job_id, group_size);
    fflush(stdout);

    free(A); 
    free(B);
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //main server
    if (rank == 0) {
        FILE *f = fopen("commands2.txt", "r");
        if (!f) {
            printf("Cannot open file\n");
            fflush(stdout);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int busy[size];
        for (int i = 1; i < size; i++)
            busy[i] = 0;

        int jobs_cnt = 0;
        int eof = 0;

        int waiting = 0;
        double wait_until = 0.0;

        char line[256];

        double start_time = MPI_Wtime();
        //fie mai avem comenzi de citit,fie mai avem raspunsuri de primit
        while (!eof || jobs_cnt > 0) {

            double now = MPI_Wtime();

            int free_worker = -1;
            for (int i = 1; i < size; i++) {
                if (!busy[i]) {
                    free_worker = i;
                    break;
                }
            }
            //daca nu suntem in wait si nu s-a terminat fisierul,citim o comanda
            if (!eof && !waiting && free_worker != -1) {
                if (fgets(line, sizeof(line), f)) {
                    printf("[DEBUG] Read line: %s", line);
                    fflush(stdout);
                    char first[32];
                    if (sscanf(line, "%31s", first) != 1) {
                        // nimic
                    }
                    else if (strcmp(first, "WAIT") == 0) {
                        int t;
                        sscanf(line, "WAIT %d", &t);
                        waiting = 1;
                        wait_until = now + t;
                        printf("[MAIN] WAIT %d seconds\n", t);
                        fflush(stdout);
                    }
                    else if (strcmp(first, "MATRIXADD") == 0 || strcmp(first, "MATRIXMULT") == 0) {
                        handle_matrix_command(line, size, busy, &jobs_cnt);
                    }
                    else if(strncmp(first, "CLI", 3) == 0){
                        Job job={0};
                        char cli[32];
                        char cmd[32];
                        char arg[100];

                        sscanf(line, "%31s %31s %99s", cli, cmd, arg);
                        sscanf(cli, "CLI%d", &job.client_id);

                        if (strcmp(cmd, "PRIMES") == 0) {
                            job.task = TASK_PRIMES;
                            job.value = atoi(arg);
                        }
                        else if (strcmp(cmd, "PRIMEDIVISORS") == 0) {
                            job.task = TASK_PRIMEDIVISORS;
                            job.value = atoi(arg);
                        }
                        else if (strcmp(cmd, "ANAGRAMS") == 0) {
                            job.task = TASK_ANAGRAMS;
                            strcpy(job.name, arg);
                        }
                        else {
                            printf("Unknown command: %s\n", cmd);
                            continue;
                        }

                        MPI_Send(&job, sizeof(Job), MPI_BYTE,
                                    free_worker, TAG_JOB, MPI_COMM_WORLD);
                        busy[free_worker] = 1;
                        jobs_cnt++;

                        printf("[MAIN] Sent job to worker %d (value=%d)\n",free_worker, job.value);
                        fflush(stdout);
                        
                    
                    }
                } else {
                    eof = 1;
                }
            }

            
            if (waiting && now >= wait_until) {
                waiting = 0;
                printf("[MAIN] WAIT finished\n");
                fflush(stdout);
            }

            //verificam daca am primit ceva
            MPI_Status status;   //contine source(cine a trimis),tag(tag-ul mesajului),si cod de eroare
            int flag;   //1 daca exista un mesaj disponibil 
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_RESULT,
                       MPI_COMM_WORLD, &flag, &status);

            if (flag) {
                Result res;
                MPI_Recv(&res, sizeof(Result), MPI_BYTE,
                         status.MPI_SOURCE, TAG_RESULT,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                busy[res.worker] = 0;
                jobs_cnt--;

                write_to_file(res);
                // if(res.result!=-1)
                //     printf("[MAIN] Result from worker %d: CLI%d -> %d\n",
                //        res.worker, res.client_id, res.result);
                // else    
                //     printf("[MAIN] Result from worker %d: CLI%d -> %s\n",
                //        res.worker, res.client_id, res.buffer);  
                // fflush(stdout);
            }
            MPI_Status mstatus;
            int mflag = 0;
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_MRESULT,MPI_COMM_WORLD, &mflag, &mstatus);
            if(mflag){
                    MatrixResult mr;
                    MPI_Recv(&mr, sizeof(MatrixResult), MPI_BYTE,
                            mstatus.MPI_SOURCE, TAG_MRESULT,
                            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    int N = mr.N;
                    int local_n = mr.local_n;

                    long long *C_part = malloc(local_n * N * sizeof(long long));
                    MPI_Recv(C_part, local_n * N, MPI_LONG_LONG,
                            mstatus.MPI_SOURCE, 0,
                            MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    MatrixTask *mt = mt_find(mr.job_id);
                    if (mt) {
                        memcpy(mt->C + mr.start_row * N,
                            C_part,
                            local_n * N * sizeof(long long));

                        mt->received_parts++;
                        if (mt->received_parts == mt->expected_parts) {
                            mt_finish(mt);
                        }
                    }

                    free(C_part);
                    busy[mr.worker] = 0;
                    jobs_cnt--;
            }
            

        }

        // Oprim workerii
        for (int i = 1; i < size; i++) {
            MPI_Send(NULL, 0, MPI_BYTE, i, TAG_STOP, MPI_COMM_WORLD);
        }
        double end_time = MPI_Wtime();
        printf("[MAIN] Parallel execution time: %.3f seconds\n",end_time - start_time);
        fflush(stdout);
        fclose(f);
        //printf("[MAIN] Server finished\n");
        
    }

   //worker
    else {
        while (1) {
            MPI_Status status;
            MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TAG_STOP) {
                MPI_Recv(NULL, 0, MPI_BYTE, 0, TAG_STOP,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                break;
            }

            if (status.MPI_TAG == TAG_JOB) {
                Job job;
                MPI_Recv(&job, sizeof(Job), MPI_BYTE,
                         0, TAG_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                Result res;
                res.client_id = job.client_id;
                res.worker=rank;
                switch (job.task) {

                    case TASK_PRIMES:
                        res.result=primes(job.value);
                        break;

                    case TASK_PRIMEDIVISORS:
                        res.result=primeDivisors(job.value);
                        break;

                    case TASK_ANAGRAMS:
                        char *buffer=anagrams(job.name);
                        strcpy(res.buffer,buffer);
                        res.result=-1;
                        free(buffer);
                        break;
                }
                MPI_Send(&res, sizeof(Result), MPI_BYTE, 0, TAG_RESULT, MPI_COMM_WORLD);

            }
            else if (status.MPI_TAG == TAG_MJOB) {
                MatrixJob mj;
                MPI_Recv(&mj, sizeof(MatrixJob), MPI_BYTE,
                        0, TAG_MJOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int N = mj.N;
                int local_n = mj.local_n;

                long long *A = malloc(local_n * N * sizeof(long long));
                long long *C = malloc(local_n * N * sizeof(long long));

                MPI_Recv(A, local_n * N, MPI_LONG_LONG,
                        0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                if (mj.task == TASK_MATRIXADD) {
                    long long *B = malloc(local_n * N * sizeof(long long));
                    MPI_Recv(B, local_n * N, MPI_LONG_LONG,
                            0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    add(C, A, B, local_n, N);
                    free(B);
                } else {
                    long long *B = malloc(N * N * sizeof(long long));
                    MPI_Recv(B, N * N, MPI_LONG_LONG,
                            0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    mult(C, A, B, local_n, N);
                    free(B);
                }

                MatrixResult mr;
                mr.job_id = mj.job_id;
                mr.worker = rank;
                mr.N = N;
                mr.start_row = mj.start_row;
                mr.local_n = local_n;

                MPI_Send(&mr, sizeof(MatrixResult), MPI_BYTE,
                        0, TAG_MRESULT, MPI_COMM_WORLD);
                MPI_Send(C, local_n * N, MPI_LONG_LONG,
                        0, 0, MPI_COMM_WORLD);

                free(A);
                free(C);
            }

        }

        printf("[WORKER %d] exiting\n", rank);
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}


/*
Results:
p=2  3.311 sec
p=4  1.232 sec
p=8  0.697 sec
*/