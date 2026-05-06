#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sched.h>

#define K 200   

struct Node 
{
    int data;
    struct Node *next;
};

struct list 
{
    struct Node *header;
    struct Node *tail;
};

pthread_mutex_t mutex_lock;
struct list *List;

void bind_thread_to_cpu(int cpuid) 
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask)) 
    {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

struct Node* generate_data_node() 
{
    struct Node *ptr = (struct Node*)malloc(sizeof(struct Node));
    if (ptr != NULL) 
    {
        ptr->next = NULL;
    } else 
    {
        printf("Node allocation failed!\n");
    }
    return ptr;
}

void *producer_thread(void *arg) 
{
    bind_thread_to_cpu(*((int *)arg));

    struct Node *local_head = NULL;
    struct Node *local_tail = NULL;
    struct Node *ptr;
    int counter = 0;

    while (counter < K) 
    {
        ptr = generate_data_node();
        if (ptr != NULL) 
        {
            ptr->data = 1;

            if (local_head == NULL) 
            {
                local_head = local_tail = ptr;
            } 
            else 
            {
                local_tail->next = ptr;
                local_tail = ptr;
            }
        }
        counter++;
    }

    pthread_mutex_lock(&mutex_lock);

    if (List->header == NULL) 
    {
        List->header = local_head;
        List->tail = local_tail;
    } 
    else 
    {
        List->tail->next = local_head;
        List->tail = local_tail;
    }

    pthread_mutex_unlock(&mutex_lock);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) 
{
    int i;
    int num_threads;
    int NUM_PROCS;
    int *cpu_array = NULL;
    struct Node *tmp, 
    struct Node *next;
    struct timeval start_time;
    struct timeval endtime;

    if (argc < 2) 
    {
        printf("Usage: %s <num_threads>\n", argv[0]);
        return 1;
    }

    num_threads = atoi(argv[1]);
    pthread_t producer[num_threads];

    NUM_PROCS = sysconf(_SC_NPROCESSORS_CONF);

    if (NUM_PROCS > 0) 
    {
        cpu_array = (int *)malloc(NUM_PROCS * sizeof(int));
        if (cpu_array == NULL) 
        {
            printf("Allocation failed!\n");
            exit(1);
        }
        for (i = 0; i < NUM_PROCS; i++) 
        {
            cpu_array[i] = i;
        }
    }

    pthread_mutex_init(&mutex_lock, NULL);

    List = (struct list *)malloc(sizeof(struct list));
    if (List == NULL) 
    {
        printf("Allocation failed!\n");
        exit(1);
    }
    List->header = List->tail = NULL;

    gettimeofday(&start_time, NULL);

    /* Create threads */
    for (i = 0; i < num_threads; i++) 
    {
        pthread_create(&producer[i], NULL, producer_thread,
                       &cpu_array[i % NUM_PROCS]);
    }

    for (i = 0; i < num_threads; i++) 
    {
        pthread_join(producer[i], NULL);
    }

    gettimeofday(&endtime, NULL);

    tmp = List->header;
    while (tmp != NULL) 
    {
        next = tmp->next;
        free(tmp);
        tmp = next;
    }

    if (cpu_array != NULL)
    {
        free(cpu_array);
    }

    free(List);

    long runtime =
        (endtime.tv_sec - start_time.tv_sec) * 1000000 +
        (endtime.tv_usec - start_time.tv_usec);

    printf("Total run time is %ld microseconds.\n", runtime);

    pthread_mutex_destroy(&mutex_lock);

    return 0;
}
