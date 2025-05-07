#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_TABLES 3
#define MAX_STUDENTS_LIMIT 100

typedef struct {
    int size;
    int seated;
    int eaten;
    int compote;
    pthread_mutex_t lock;
    pthread_cond_t full;
    pthread_cond_t compote_ready;
} Table;

Table tables[MAX_TABLES];
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

void reset_tables() {
    for (int i = 0; i < MAX_TABLES; i++) {
        tables[i].seated = 0;
        tables[i].eaten = 0;
        tables[i].compote = 0;
    }
}

void* student_thread(void* arg) {
    int id = (int)(long)arg;
    Table* chosen_table = NULL;

    pthread_mutex_lock(&global_lock);
    int min_gap = 99;

    for (int i = 0; i < MAX_TABLES; i++) {
        int gap = tables[i].size - tables[i].seated;
        if (gap > 0 && gap < min_gap) {
            min_gap = gap;
            chosen_table = &tables[i];
        }
    }

    if (chosen_table == NULL) {
        printf("Student %d: no seat available!\n", id);
        pthread_mutex_unlock(&global_lock);
        return NULL;
    }

    pthread_mutex_lock(&chosen_table->lock);
    chosen_table->seated++;
    printf("Student %d sits at table for %d. (%d/%d)\n", id, chosen_table->size, chosen_table->seated, chosen_table->size);
    if (chosen_table->seated == chosen_table->size) {
        pthread_cond_broadcast(&chosen_table->full);
    }
    pthread_mutex_unlock(&chosen_table->lock);
    pthread_mutex_unlock(&global_lock);

    pthread_mutex_lock(&chosen_table->lock);
    while (chosen_table->seated < chosen_table->size) {
        pthread_cond_wait(&chosen_table->full, &chosen_table->lock);
    }
    pthread_mutex_unlock(&chosen_table->lock);

    printf("Student %d is eating...\n", id);
    sleep(1);

    pthread_mutex_lock(&chosen_table->lock);
    chosen_table->eaten++;
    if (chosen_table->eaten == chosen_table->size) {
        printf("Table of %d: Everyone done eating. Serving compote!\n", chosen_table->size);
        pthread_cond_broadcast(&chosen_table->compote_ready);
    }
    pthread_mutex_unlock(&chosen_table->lock);

    pthread_mutex_lock(&chosen_table->lock);
    while (chosen_table->eaten < chosen_table->size) {
        pthread_cond_wait(&chosen_table->compote_ready, &chosen_table->lock);
    }
    pthread_mutex_unlock(&chosen_table->lock);

    printf("Student %d is drinking compote...\n", id);
    sleep(1);

    pthread_mutex_lock(&chosen_table->lock);
    chosen_table->compote++;
    if (chosen_table->compote == chosen_table->size) {
        printf("Table of %d: Everyone finished compote. Leaving together.\n", chosen_table->size);
    }
    pthread_mutex_unlock(&chosen_table->lock);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_students>\n", argv[0]);
        return 1;
    }

    int students_requested = atoi(argv[1]);
    if (students_requested < 1 || students_requested > MAX_STUDENTS_LIMIT) {
        printf("Please choose a number of students between 1 and %d.\n", MAX_STUDENTS_LIMIT);
        return 1;
    }

    srand(time(NULL));

    tables[0].size = 3;
    tables[1].size = 4;
    tables[2].size = 5;
    for (int i = 0; i < MAX_TABLES; i++) {
        pthread_mutex_init(&tables[i].lock, NULL);
        pthread_cond_init(&tables[i].full, NULL);
        pthread_cond_init(&tables[i].compote_ready, NULL);
    }

    for (int run = 1; run <= 10; run++) {
        printf("\n===== Simulation Run %d =====\n", run);
        pthread_t students[students_requested];
        reset_tables();

        for (int i = 0; i < students_requested; i++) {
            pthread_create(&students[i], NULL, student_thread, (void*)(long)i);
            usleep(100000);
        }

        for (int i = 0; i < students_requested; i++) {
            pthread_join(students[i], NULL);
        }

        printf("Run %d complete: all students have finished lunch.\n", run);
        sleep(1);
    }

    return 0;
}
