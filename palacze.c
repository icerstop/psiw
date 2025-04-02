#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdbool.h>

#define NUM_SMOKERS 3

struct message {
    long msg_type;
    int ingredient;
    int from_smoker;
    float price;
};

typedef struct {
    int id;
    float balance;
    int ingredient;
} Smoker;

typedef struct {
    float tobacco_price;
    float paper_price;
    float match_price;
} Agent;

Agent agent;
Smoker smokers[NUM_SMOKERS];
int msgid;

sem_t sem_agent;
sem_t sem_smokers[NUM_SMOKERS];

void *agent_routine(void *arg);
void *smoker_routine(void *arg);
void send_prices_to_smokers();
void handle_ingredient_offer(int ingredient, float price, int from_smoker);
void make_ingredient_offer(Smoker *smoker);
void buy_ingredient(Smoker *smoker, int ingredient, float price);

void *agent_routine(void *arg) {
    while (true) {
        sem_wait(&sem_agent);
        agent.tobacco_price = (rand() % 100) / 10.0;
        agent.paper_price = (rand() % 100) / 10.0;
        agent.match_price = (rand() % 100) / 10.0;
        sem_post(&sem_agent);
        sleep(1);
    }
}

void *smoker_routine(void *arg) {
    Smoker *smoker = (Smoker *)arg;
    struct message msg;

    while (true) {
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 1, IPC_NOWAIT) != -1) {
        sem_wait(&sem_agent);
        float cost_of_missing_ingredients = agent.tobacco_price + agent.paper_price + agent.match_price;
        switch (smoker->ingredient) {
        case 0: cost_of_missing_ingredients -= agent.tobacco_price; break;
        case 1: cost_of_missing_ingredients -= agent.paper_price; break;
        case 2: cost_of_missing_ingredients -= agent.match_price; break;
    }

    if (msg.ingredient != smoker->ingredient && smoker->balance >= cost_of_missing_ingredients) {
        buy_ingredient(smoker, msg.ingredient, cost_of_missing_ingredients);
    }
    sem_post(&sem_agent);
}

        sleep(1);
    }
}

void make_ingredient_offer(Smoker *smoker) {
    sem_wait(&sem_agent);
    struct message msg;
    msg.msg_type = 1;
    msg.ingredient = smoker->ingredient;
    msg.from_smoker = smoker->id;
    
    switch (smoker->ingredient) {
        case 0: msg.price = agent.tobacco_price; break;
        case 1: msg.price = agent.paper_price; break;
        case 2: msg.price = agent.match_price; break;
    }

    sem_post(&sem_agent);

    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Offer sending failed");
    }
}

void handle_ingredient_offer(int ingredient, float price, int from_smoker) {
    for (int i = 0; i < NUM_SMOKERS; i++) {
        if (smokers[i].id != from_smoker && smokers[i].ingredient != ingredient) {
            buy_ingredient(&smokers[i], ingredient, price);
            break;
        }
    }
}

void buy_ingredient(Smoker *smoker, int ingredient, float price) {
    sem_wait(&sem_agent);
    float totalCost = agent.tobacco_price + agent.paper_price + agent.match_price;
    switch (smoker->ingredient) {
        case 0: totalCost -= agent.tobacco_price; break;
        case 1: totalCost -= agent.paper_price; break;
        case 2: totalCost -= agent.match_price; break;
    }

    if (smoker->balance >= totalCost) {
        smoker->balance -= totalCost;
        printf("Smoker %d bought both ingredients for %.2f. New balance: %.2f\n", smoker->id, totalCost, smoker->balance);
    } else {
        printf("Smoker %d cannot afford both ingredients. Required: %.2f, Balance: %.2f\n", smoker->id, totalCost, smoker->balance);
    }
    sem_post(&sem_agent);
}

int main() {
    pthread_t agent_thread, smoker_threads[NUM_SMOKERS];
    key_t key;
    int i;

    key = ftok("progfile", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    sem_init(&sem_agent, 0, 1);
    for (i = 0; i < NUM_SMOKERS; i++) {
        sem_init(&sem_smokers[i], 0, 0);
    }

    for (i = 0; i < NUM_SMOKERS; i++) {
        smokers[i].id = i;
        smokers[i].balance = 100.0;
        smokers[i].ingredient = i % 3;
    }

    if (pthread_create(&agent_thread, NULL, &agent_routine, NULL) != 0) {
        perror("Failed to create agent thread");
        return 1;
    }

    for (i = 0; i < NUM_SMOKERS; i++) {
        if (pthread_create(&smoker_threads[i], NULL, &smoker_routine, (void *)&smokers[i]) != 0) {
            perror("Failed to create smoker thread");
            return 1;
        }
    }

    pthread_join(agent_thread, NULL);
    for (i = 0; i < NUM_SMOKERS; i++) {
        pthread_join(smoker_threads[i], NULL);
    }

    msgctl(msgid, IPC_RMID, NULL);
    sem_destroy(&sem_agent);
    for (i = 0; i < NUM_SMOKERS; i++) {
        sem_destroy(&sem_smokers[i]);
    }

    return 0;
}
