#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

// Stałe konfiguracyjne dla symulacji
#define MAX_TUGBOATS 10       // Maksymalna liczba dostępnych holowników
#define MAX_DOCKS 5           // Maksymalna liczba dostępnych doków
#define SHIP_COUNT 15         // Liczba statków uczestniczących w symulacji
#define TUGBOAT_CAPACITY 200  // Maksymalna waga obsługiwana przez jeden holownik
#define TIMEOUT_SECONDS 5     // Czas oczekiwania (w sekundach) przed ponowną próbą uzyskania zasobów

// Struktura reprezentująca statek
typedef struct {
    int id;                 // Unikalny identyfikator statku
    int weight;             // Waga statku
    int tugboats_needed;    // Liczba holowników potrzebnych do obsługi statku
    int docked;             // Status dokowania (1 - zadokowany, 0 - niezadokowany)
} Ship;

// Struktura reprezentująca port
typedef struct {
    sem_t tugboats;  // Semafor kontrolujący dostęp do holowników
    sem_t docks;     // Semafor kontrolujący dostęp do doków
} Port;

Port port;  // Globalna instancja portu używana przez wszystkie wątki

// Funkcja wykonywana przez wątek reprezentujący statek
void* ship_routine(void* arg) {
    Ship* ship = (Ship*)arg;  // Rzutowanie argumentu na typ Ship
    int acquired_tugboats = 0;  // Licznik zdobytych holowników

    // Główna pętla wątku statku
    while (1) {
        // Próba zajęcia doku - używając semafora
        if (sem_trywait(&port.docks) == 0) {
            // Pętla próbująca zdobyć wymaganą liczbę holowników
            while (acquired_tugboats < ship->tugboats_needed) {
                if (sem_trywait(&port.tugboats) == 0) {
                    acquired_tugboats++;  // Inkrementacja licznika zdobytych holowników
                } else {
                    // Brak dostępnych holowników; przerwanie pętli
                    break;
                }
            }

            // Sprawdzenie, czy statek zdobył wszystkie wymagane zasoby
            if (acquired_tugboats == ship->tugboats_needed) {
                // Zadokowanie - wyświetlenie informacji o dokowaniu
                printf("Statek ID %d o wadze %d zadokował, używając %d holowników.\n", ship->id, ship->weight, ship->tugboats_needed);

                sleep(rand() % 5 + 1);  // Symulacja czasu spędzonego na doku

                // Opuszczenie doku - wyświetlenie informacji o opuszczeniu
                printf("Statek ID %d opuszcza port, zwalniając %d holowników.\n", ship->id, ship->tugboats_needed);

                // Zwolnienie zajętych zasobów
                sem_post(&port.docks);  // Zwolnienie doku
                for (int i = 0; i < ship->tugboats_needed; i++) {
                    sem_post(&port.tugboats);  // Zwolnienie holowników
                }
                break;  // Wyjście z pętli - zakończenie pracy wątku
            } else {
                // Statek nie zdobył wszystkich zasobów - zwolnienie zajętych
                sem_post(&port.docks);  // Zwolnienie doku
                for (int i = 0; i < acquired_tugboats; i++) {
                    sem_post(&port.tugboats);  // Zwolnienie holowników
                }
                acquired_tugboats = 0;  // Reset licznika zdobytych holowników
            }
        }

        sleep(TIMEOUT_SECONDS);  // Odczekanie przed kolejną próbą
    }

    return NULL;
}

// Główna funkcja programu
int main() {
    pthread_t ships[SHIP_COUNT];  // Tablica wątków dla każdego statku
    Ship ship_data[SHIP_COUNT];   // Tablica danych dla każdego statku
    srand(time(NULL));  // Inicjalizacja generatora liczb losowych

    // Inicjalizacja semaforów
    sem_init(&port.tugboats, 0, MAX_TUGBOATS);  // Inicjalizacja semafora holowników
    sem_init(&port.docks, 0, MAX_DOCKS);        // Inicjalizacja semafora doków

    // Tworzenie wątków dla statków
    for (int i = 0; i < SHIP_COUNT; i++) {
        ship_data[i].id = i + 1;  // Przypisanie ID do statku
        ship_data[i].weight = (rand() % 901 + 100);  // Losowanie wagi statku
        // Obliczenie liczby potrzebnych holowników
        ship_data[i].tugboats_needed = (ship_data[i].weight + TUGBOAT_CAPACITY - 1) / TUGBOAT_CAPACITY;
        ship_data[i].docked = 0;  // Ustawienie statusu dokowania na niezadokowany
        // Utworzenie wątku dla statku
        pthread_create(&ships[i], NULL, ship_routine, (void*)&ship_data[i]);
    }

    // Oczekiwanie na zakończenie wszystkich wątków statków
    for (int i = 0; i < SHIP_COUNT; i++) {
        pthread_join(ships[i], NULL);
    }

    // Zwalnianie zasobów semaforów
    sem_destroy(&port.tugboats);  // Zwalnianie semafora holowników
    sem_destroy(&port.docks);     // Zwalnianie semafora doków

    return 0;
}
