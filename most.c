#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>

enum direction {
    north, south
};
int semid = 0;
int n_id = 0;



//operacje semaforowe
static struct sembuf buf;
void semV(int semid, int s_num);
void semP(int semid, int s_num);

void semV(int semid, int s_num) {
    buf.sem_num = s_num;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1) {
        perror("V sem operation");
        exit(1);
    }
}


void semP(int semid, int s_num) {
    buf.sem_num = s_num;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1) {
        perror("P sem operation");
        exit(1);
    }
}


int process(enum direction dir) {
    while (1) {
        int *n = (int *) shmat(n_id, NULL, 0);
        if (n == NULL) {
            perror("shared memory addition "); //globalna pamiec wspoldzielona
            exit(1);
        }

        //zwiększenie ilości samochodów
        semP(semid, 0);

        if (n[north] == 0 && n[south] == 1) //samochód pojawia się jako pierwszy na moście
            semP(semid, 2 - dir);
        n[dir]++;
        semV(semid, 0);

        //próba wjazdu na most
        semP(semid, 1 + dir);
        semV(semid, 1 + dir);
        switch (dir) {
            case north:
                printf("Przejazd na polnoc\n");
                break;
            case south:
                printf("Przejazd na poludnie\n");
                break;
        }

        semP(semid, 0);
        n[dir]--;
        //ostatni samochód wyjeżdża, zezwala na wjazd samochodów w drugim kierunku
        if (n[dir] == 0)
            semV(semid, 2 - dir);
        semV(semid, 0);

        return 1;
    }
}

int main(int argc,
         char **argv) { //wywołanie funkcji ./main [liczba samochodów jadąca na północ] [liczba samochodów jadących na południe]
    srand(time(NULL));
    if (argc < 3) {
        printf("argument error\n");
        return 1;
    }
    int cars[2];
    cars[north] = atoi(argv[1]);
    cars[south] = atoi(argv[2]);

    //semafory
    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0600);
    if (semid == -1) {
        perror("semaphore creation");
        return 1;
    }
    semctl(semid, 0, SETVAL,
           1); //semafor odpowiadający za dostęp do sekcji krytycznej zliczającej ilość samochodów jadących na północ i południe
    semctl(semid, 1, SETVAL, 1); //semafor blokujący jazdę na północ
    semctl(semid, 2, SETVAL, 1); //semafor blokujący jazdę na południe

    //pamięć współdzielona
    n_id = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    //n[0] - samochody jadące na północ
    //n[1] - samochody jadące na południe
    if (n_id == -1) {
        perror("shared memory creation");
        return 1;
    }
    int *n = (int *) shmat(n_id, NULL, 0);
    if (n == NULL) {
        perror("shared memory addition ");
        return 1;
    }
    n[north] = 0;
    n[south] = 0;

    for(int i = 0; i < cars[south]; i++){ // puszczanie procesow dla aut jadacych na poludnie
        if(fork() == 0){
            process(south);
            return 1;
        }
    }

    for(int i = 0; i < cars[north]; i++){ // puszczanie procesow dla aut jadacych na poludnie
        if(fork() == 0){
            process(north);
            return 1;
        }
    }

    return 1;
}




