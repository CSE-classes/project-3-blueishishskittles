#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 5
#define END_OF_STREAM '\0'

char buffer[BUFFER_SIZE];
int in = 0, out = 0, count = 0;

pthread_mutex_t lock;
pthread_cond_t not_full;
pthread_cond_t not_empty;

FILE *fp;

void *producer(void *arg) {
    int ch;

    while ((ch = fgetc(fp)) != EOF) {
        pthread_mutex_lock(&lock);

        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&not_full, &lock);
        }

        buffer[in] = (char)ch;
        in = (in + 1) % BUFFER_SIZE;
        count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&lock);
    }

    pthread_mutex_lock(&lock);
    while (count == BUFFER_SIZE) {
        pthread_cond_wait(&not_full, &lock);
    }

    buffer[in] = END_OF_STREAM;
    in = (in + 1) % BUFFER_SIZE;
    count++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

void *consumer(void *arg) {
    char ch;

    while (1) {
        pthread_mutex_lock(&lock);

        while (count == 0) {
            pthread_cond_wait(&not_empty, &lock);
        }

        ch = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&lock);

        if (ch == END_OF_STREAM)
            break;

        printf("%c", ch);
        fflush(stdout);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t prod, cons;

    fp = fopen("message.txt", "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        return -1;
    }

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&not_full, NULL);
    pthread_cond_init(&not_empty, NULL);

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    fclose(fp);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}
