#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_JOB     1
#define TAG_RESULT  2
#define TAG_STOP    3

typedef struct {
    int client_id;
    int value;
} Job;

typedef struct {
    int client_id;
    int result;
    int worker;
} Result;

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //main server
    if (rank == 0) {
        FILE *f = fopen("commands.txt", "r");
        if (!f) {
            printf("Cannot open file\n");
            fflush(stdout);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int worker_busy[size];
        for (int i = 1; i < size; i++)
            worker_busy[i] = 0;

        int jobs_running = 0;
        int eof = 0;

        int waiting = 0;
        double wait_until = 0.0;

        char line[256];

        //fie mai avem comenzi de citit,fie mai avem raspunsuri de primit
        while (!eof || jobs_running > 0) {

            double now = MPI_Wtime();

            //daca nu suntem in wait si nu s-a terminat fisierul,citim o comanda
            if (!eof && !waiting) {
                if (fgets(line, sizeof(line), f)) {

                    if (strncmp(line, "WAIT", 4) == 0) {
                        int t;
                        sscanf(line, "WAIT %d", &t);
                        waiting = 1;
                        wait_until = now + t;
                        printf("[MAIN] WAIT %d seconds\n", t);
                        fflush(stdout);
                    } else {
                        Job job;
                        char cmd[32];
                        char cli[32];

                        sscanf(line, "%s %s %d", cli, cmd, &job.value);
                        sscanf(cli, "CLI%d", &job.client_id);

                        //daca gasim un worker liber,ii trimitem job-ul
                        for (int w = 1; w < size; w++) {
                            if (!worker_busy[w]) {
                                MPI_Send(&job, sizeof(Job), MPI_BYTE,
                                         w, TAG_JOB, MPI_COMM_WORLD);
                                worker_busy[w] = 1;
                                jobs_running++;

                                printf("[MAIN] Sent job to worker %d (value=%d)\n",w, job.value);
                                fflush(stdout);
                                break;
                            }
                        }
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

                worker_busy[res.worker] = 0;
                jobs_running--;

                printf("[MAIN] Result from worker %d: CLI%d -> %d\n",
                       res.worker, res.client_id, res.result);
                fflush(stdout);
            }
        }

        // Oprim workerii
        for (int w = 1; w < size; w++) {
            MPI_Send(NULL, 0, MPI_BYTE, w, TAG_STOP, MPI_COMM_WORLD);
        }

        fclose(f);
        printf("[MAIN] Server finished\n");
        fflush(stdout);
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

                int result = job.value * job.value;  //square

                Result res;
                res.client_id = job.client_id;
                res.result = result;
                res.worker = rank;

                MPI_Send(&res, sizeof(Result), MPI_BYTE,
                         0, TAG_RESULT, MPI_COMM_WORLD);
            }
        }

        printf("[WORKER %d] exiting\n", rank);
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}
dfzzzzzzzzzzzzzzzzzzzzzzzsdeftggdd