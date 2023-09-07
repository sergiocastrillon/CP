#include <stdlib.h>
#include <threads.h>

// circular array
typedef struct _queue {
    mtx_t mtx_queue;
    int size;
    int used;
    int first;
    void **data;
} _queue;

#include "queue.h"

queue q_create(int size) {
    queue q = malloc(sizeof(_queue));

    mtx_init(&q->mtx_queue,mtx_plain);

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
    if(q->size == q->used){
        mtx_unlock(&q->mtx_queue);
        return -1;
    } 

    q->data[(q->first + q->used) % q->size] = elem;
    q->used++;

    mtx_unlock(&q->mtx_queue);
    return 0;
}

void *q_remove(queue q) {

    mtx_lock(&q->mtx_queue);
    void *res;
    if(q->used == 0){
        mtx_unlock(&q->mtx_queue);
        return NULL;
    } 

    res = q->data[q->first];

    q->first = (q->first + 1) % q->size;
    q->used--;

    mtx_unlock(&q->mtx_queue);
    
    return res;
}

void q_destroy(queue q) {
    mtx_destroy(&q->mtx_queue);
    free(q->data);
    free(q);
}
