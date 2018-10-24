
#include "queue.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*
 * Struct for the queue
 */
typedef struct QueueStruct
{
	size_t size;
	pthread_mutex_t mutex_lock;
	sem_t enqueue;
    sem_t dequeue;
	void **data;
	int head;
	int tail;
} Queue;

/*
 * Init the queue and allocate memory for it
 */
Queue *queue_alloc(int size) 
{
	//allocate queue struct memory
    Queue *queue = malloc(sizeof(Queue)); 
    
    //initiate values
    queue->size = size;
    queue->data = malloc(size * sizeof(void*));
	queue->head = 0; //location of head and tail nodes
	queue->tail = 0;
	
	//init semaphores
	pthread_mutex_init(&queue->mutex_lock, NULL);
	sem_init(&queue->enqueue, 0, size);
    sem_init(&queue->dequeue, 0, 0);
	return queue;
}

/*
 * Free the queue and all other mem resources
 */
void queue_free(Queue *queue) 
{
	//destroy semaphores
	pthread_mutex_destroy(&queue->mutex_lock);
	sem_destroy(&queue->enqueue);
	sem_destroy(&queue->dequeue);
	
	//free queue
	free(queue->data);
	free(queue);
	return;
}

/*
 * Append to the FIFO queue
 */
void queue_put(Queue *queue, void *item) 
{
	//block if queue is full, decrementing sem
    sem_wait(&queue->enqueue);

    //lock and put item to the tail of queue
    pthread_mutex_lock(&queue->mutex_lock);
    queue->data[queue->tail] = item;
    //modulus so can reuse memory from popped data like circ buff
    queue->tail = (queue->tail + 1)%queue->size; 
    
    //increment sem and unlock
    pthread_mutex_unlock(&queue->mutex_lock);
    sem_post(&queue->dequeue);
    return;
}

/*
 * Pop from the front of the FIFO queue
 */
void *queue_get(Queue *queue) 
{
	//block if queue is empty
    sem_wait(&queue->dequeue);

    //lock thread and pop item
    pthread_mutex_lock(&queue->mutex_lock);
    void *item = queue->data[queue->head];
    //modulus so can reuse memory from popped data like circ buff
    queue->head = (queue->head + 1)%queue->size;

	//increment sem and unlock
    pthread_mutex_unlock(&queue->mutex_lock);
    sem_post(&queue->enqueue);
    return item;
}
