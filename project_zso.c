#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_RUNS 10
#define STUDENTS 15

#ifdef DEBUG
#define SLEEP(x) sleep(x)
#define LOG(...) printf(__VA_ARGS__)
#else
#define SLEEP(x)
#define LOG(...)
#endif

typedef struct {
    int size;
    int seated;
    int eaten;
    int compote;
    pthread_mutex_t lock;
    pthread_cond_t full;
    pthread_cond_t compote_ready;
} Table;

#define TABLE_COUNT 3
const int TABLE_SIZES[TABLE_COUNT] = {3, 4, 5};

Table tables[TABLE_COUNT];
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

void reset_tables() {
    for (int i = 0; i < TABLE_COUNT; i++) {
        tables[i].seated = 0;
        tables[i].eaten = 0;
        tables[i].compote = 0;
    }
}

void* student_thread(void* arg) {
    int id = (int)(long)arg;
    Table* chosen = NULL;

    pthread_mutex_lock(&global_lock);

    int min_gap = 999;
    for (int i = 0; i < TABLE_COUNT; i++) {
        int gap = tables[i].size - tables[i].seated;
        if (gap > 0 && gap < min_gap) {
            min_gap = gap;
            chosen = &tables[i];
        }
    }

    if (!chosen) {
        LOG("Student %d: no available seat.\n", id);
        pthread_mutex_unlock(&global_lock);
        return NULL;
    }

    pthread_mutex_lock(&chosen->lock);
    chosen->seated++;
    LOG("Student %d sits at table %d (%d/%d)\n", id, chosen->size, chosen->seated, chosen->size);
    if (chosen->seated == chosen->size) {
        pthread_cond_broadcast(&chosen->full);
    }
    pthread_mutex_unlock(&chosen->lock);
    pthread_mutex_unlock(&global_lock);

    pthread_mutex_lock(&chosen->lock);
    while (chosen->seated < chosen->size) {
        pthread_cond_wait(&chosen->full, &chosen->lock);
    }
    pthread_mutex_unlock(&chosen->lock);

    LOG("Student %d is eating...\n", id);
    SLEEP(1);

    pthread_mutex_lock(&chosen->lock);
    chosen->eaten++;
    if (chosen->eaten == chosen->size) {
        LOG("Table for %d: all done eating. Compote time!\n", chosen->size);
        pthread_cond_broadcast(&chosen->compote_ready);
    }
    pthread_mutex_unlock(&chosen->lock);

    pthread_mutex_lock(&chosen->lock);
    while (chosen->eaten < chosen->size) {
        pthread_cond_wait(&chosen->compote_ready, &chosen->lock);
    }
    pthread_mutex_unlock(&chosen->lock);

    LOG("Student %d is drinking compote...\n", id);
    SLEEP(1);

    pthread_mutex_lock(&chosen->lock);
    chosen->compote++;
    if (chosen->compote == chosen->size) {
        LOG("Table for %d: all finished compote. Leaving together.\n", chosen->size);
    }
    pthread_mutex_unlock(&chosen->lock);

    return NULL;
}

void project_zso(void) {
    pthread_t students[STUDENTS];

    // Initialize tables if not done
    static int initialized = 0;
    if (!initialized) {
        for (int i = 0; i < TABLE_COUNT; i++) {
            tables[i].size = TABLE_SIZES[i];
            pthread_mutex_init(&tables[i].lock, NULL);
            pthread_cond_init(&tables[i].full, NULL);
            pthread_cond_init(&tables[i].compote_ready, NULL);
        }
        initialized = 1;
    }

    for (int round = 1; round <= NUM_RUNS; round++) {
        LOG("\n==== RUN %d ====\n", round);
        reset_tables();

        // Create all student threads
        for (int i = 0; i < STUDENTS; i++) {
            pthread_create(&students[i], NULL, student_thread, (void*)(long)i);
        }

        // Join all threads
        for (int i = 0; i < STUDENTS; i++) {
            pthread_join(students[i], NULL);
        }

        LOG("==== END OF RUN %d ====\n", round);
        SLEEP(1);
    }
}

int main() {
    project_zso();
    return 0;
}

