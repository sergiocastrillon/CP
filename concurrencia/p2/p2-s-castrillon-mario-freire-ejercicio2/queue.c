#include <stdlib.h>
#include <threads.h>
#include <stdbool.h>

// circular array
typedef struct _queue {
    mtx_t mtx_queue;
    int size;
    int used;
    int first;
    void **data;
    cnd_t full;
    cnd_t empty;
    bool final;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));

    mtx_init(&q->mtx_queue,mtx_plain);
    cnd_init(&q->full);
    cnd_init(&q->empty);
    q->final = false;

    q->size  = size;
    q->used  = 0;
    q->first = 0;
    q->data  = malloc(size * sizeof(void *));

    return q;
}

int q_elements(queue q) {
    mtx_lock(&q->mtx_queue);
    int r = q->used;
    mtx_unlock(&q->mtx_queue);
    return r;
}

int q_insert(queue q, void *elem) {
    mtx_lock(&q->mtx_queue);

    if(q->final){
      mtx_unlock(&q->mtx_queue);
      return -1;
    }
    while(q->size == q->used){
        cnd_wait(&q->full,&q->mtx_queue);
    } 

    q->data[(q->first + q->used) % q->size] = elem;
    q->used++;

    if(q->used == 1) cnd_broadcast(&q->empty);
    mtx_unlock(&q->mtx_queue);

    return 0;
}

void *q_remove(queue q) {
    void *res;
    mtx_lock(&q->mtx_queue);
    if(q->final && q->used == 0){
      mtx_unlock(&q->mtx_queue);
      return NULL;
    }
    while(q->used == 0){
        cnd_wait(&q->empty,&q->mtx_queue);
        if(q->final){
          mtx_unlock(&q->mtx_queue);
          return NULL;
        }
    } 
    res = q->data[q->first];

    q->first = (q->first + 1) % q->size;
    q->used--;

    if(q->used == q->size - 1) cnd_broadcast(&q->full);

    mtx_unlock(&q->mtx_queue);
    
    return res;
}

void q_destroy(queue q) {
    mtx_destroy(&q->mtx_queue);
    cnd_destroy(&q->full);
    cnd_destroy(&q->empty);
    free(q->data);
    free(q);
}

void q_final(queue q){
    mtx_lock(&q->mtx_queue);
    q->final = true;
    cnd_broadcast(&q->empty); // Por si se quedan hilos pendientes
    mtx_unlock(&q->mtx_queue);
}
