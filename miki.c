#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

static volatile int reindeer_waiting = 0; // co to robiło?
static volatile int elves_waiting = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  santa_sleep = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  reindeer_sleep = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  elf_sleep = PTHREAD_COND_INITIALIZER;


void elf_action(int n_elf){
    printf("\tMikołaj poszedł sprawdzić jak idzie praca u %d skrzatów.\nCzekające renifery: %d\n", n_elf, reindeer_waiting); fflush(stdout);
    sleep(rand()%5);
    
   
    
}
void reindeer_action(void){
    printf("\tMikołaj wziął renifery i zaprzągł je do sań. \nCzekające skrzaty: %d \n", elves_waiting); fflush(stdout);
    sleep(rand()%5);

}


void* santa_clause(void* p){
    
    // lock mutexa
    pthread_mutex_lock( &mtx );
    printf("\tMikołaj idzie spać\n"); fflush(stdout);

    // mikołaj jest zawsze w sekcji krytycznej 
    while (true){
        // go to sleep
        pthread_cond_wait(&santa_sleep, &mtx);
        printf("\tMikołaj obudził się\n"); fflush(stdout);


        do {
            // sekcja krytyczna
            if ( reindeer_waiting == 9){
                
                pthread_mutex_unlock( &mtx ); // renifery wychodzą z sekcji krytycznej, żeby skrzaty mogły ustawić się w kolejce

                reindeer_action(); // zaprzęganie reniferów

                pthread_mutex_lock( &mtx ); // CO TU SIE DZIEJE
                
                reindeer_waiting = 0; // wypuszczanie reniferów
                pthread_cond_broadcast(&reindeer_sleep);


            } else if (elves_waiting > 2){
                int tmp = elves_waiting;
                

                pthread_mutex_unlock( &mtx );
                
                elf_action(tmp); // sprawdzanie pracy u skrzatów

                pthread_mutex_lock( &mtx );

                elves_waiting = 0;
                pthread_cond_broadcast(&elf_sleep);
            }
        } while (elves_waiting >= 9); // pętla zapewniająca że gdy skrzatów i reniferów będzie 9, mikołaj po zaprzęgnięciu reniferów zajmie się też skrzatami
        //bez wybudzenia skrzatów (ponieważ nie ma kolejnego by to zrobić)
        printf("\tMikołaj idzie spać\n"); fflush(stdout);

    }
}


void* reindeer(void* p){
    while (1){
        printf("*patataj patataj\n"); fflush(stdout);
        
        pthread_mutex_lock( &mtx );
        // Do kolejki dołącza renifer
        reindeer_waiting++;
        if (reindeer_waiting == 9){
            printf("REINDEER Waking up santa\n"); fflush(stdout);
            pthread_cond_signal(&santa_sleep);
        }
        // Mikołaj dalej śpi ponieważ mtx zajmuje renifer
        pthread_cond_wait(&reindeer_sleep, &mtx);
        
        pthread_mutex_unlock( &mtx );
        
        // Renifer wychodzi z sekcji krytycznej
        sleep(rand()%2+1);
    }
}


void* elf(void* p){
    while (1){
        
        pthread_mutex_lock( &mtx );
        printf("elf joining queue of len %d\n", elves_waiting); fflush(stdout);
        elves_waiting++;
        if (elves_waiting >= 3){
            printf("ELF Waking up santa %d waiting\n", elves_waiting); fflush(stdout);
            pthread_cond_signal(&santa_sleep);
        }
        // Mikołaj dalej śpi ponieważ mtx zajmuje skrzat
        pthread_cond_wait(&elf_sleep, &mtx);
        
        pthread_mutex_unlock( &mtx );
        
        printf("Skrzat wychodzi z sekcji krytycznej\n"); fflush(stdout);
        usleep((rand()%4+1)*500*1000);
    }
}



int main(void)
{
    srand(time(NULL));

    pthread_t santa;
    pthread_t reindeers[9];
    pthread_t elves[9];

    pthread_create(&santa, NULL, santa_clause, NULL); // tworzenie procesu Mikołaja

    for (size_t i = 0; i < 9; i++) // tworzenie reniferów
    {
        pthread_create(&reindeers[i], NULL, reindeer, NULL);
    }
    
    for (size_t i = 0; i < 9; i++) // tworzenie skrzatów
    {
        pthread_create(&elves[i], NULL, elf, NULL);
    }
    pthread_join(santa, NULL);
}
