#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include "options.h"

#define DELAY_SCALE 1000


struct array {
    int size;
    int *arr;
    mtx_t* mutex;
};

struct args {
    int id;
    int delay;
    int iterations;
    struct array* arr;
};


void apply_delay(int delay) {
    for(int i = 0; i < delay * DELAY_SCALE; i++); // waste time
}

// Los hilos deberían ejecutar esta función (es decir thr_create(_,increment,_))
// Mod
int increment(void * p_arg)
{
    
    // Obtain arguments from struct arg
    struct args *a = p_arg;
    struct array* arr = a->arr;
    int id = (a->id);
    int delay = a->delay;
    int iterations = a->iterations;
    mtx_t *mutex_arr = arr->mutex; // Apunta a un array
    //

    int pos, val;
    for(int i = 0; i < iterations; i++) {

        pos = rand() % arr->size;
        printf("%d increasing position %d\n", id, pos);
        
        // Bloqueo
        mtx_lock(&mutex_arr[pos]);

        val = arr->arr[pos];
        apply_delay(delay);

        val ++;
        apply_delay(delay);

        arr->arr[pos] = val;

        //Desbloqueo
        mtx_unlock(&mutex_arr[pos]);
        apply_delay(delay); // Hay que dejar delay para que le de tiempo a otro thread
        // a obtener el mutex

        
        
    }

    return 0;
}


void print_array(struct array arr) {
    int total = 0;

    for(int i = 0; i < arr.size; i++) {
        total += arr.arr[i];
        printf("%d ", arr.arr[i]);
    }

    printf("\nTotal: %d\n", total);
}

// Inicia los hilos
void do_operations(struct options opt, struct array arr){

    thrd_t id_c[opt.num_threads];

    mtx_t *aux;
    if((aux = malloc(opt.size * sizeof(mtx_t))) == NULL){
        printf("Error reservando memoria, saliendo\n");
        exit(1);
    }
    for(int i = 0; i < opt.size; i++){
        mtx_init(&aux[i], mtx_plain);
    }
    
    arr.mutex = aux;

    struct args *arg_array[opt.num_threads];

    for(int i = 0; i < opt.num_threads; i++){
        struct args* arg = malloc(sizeof(struct args));
        arg_array[i] = arg; // Guardamos los structs para poder liberarlos

        arg->arr = &arr;
        arg->delay = opt.delay;
        arg->id = i;
        arg->iterations = opt.iterations;
    
        if(thrd_create(&id_c[i],increment,arg) != 0){
            printf("Error creando el thread %d, saliendo\n",i);
            exit(1);
        };
    }

    for(int i = 0; i < opt.num_threads; i++){
        thrd_join(id_c[i], NULL);
        free(arg_array[i]);
    }

    for(int i = 0; i < opt.size; i++) mtx_destroy(&aux[i]);
    free(aux);
}


int main (int argc, char **argv)
{
    struct options       opt;
    struct array         arr;

    srand(time(NULL));

    // Default values for the options
    opt.num_threads  = 5; // Hilos que se tienen que crear
    opt.size         = 10; // Tamaño de array
    opt.iterations   = 100; // Iteraciones que realiza cada thread
    opt.delay        = 1000; // Tiempo de espera en operación apply_delay

    read_options(argc, argv, &opt);

    arr.size = opt.size;
    if((arr.arr  = malloc(arr.size * sizeof(int))) == NULL){
        printf("Error reservando memoria, saliendo\n");
        exit(1);
    };

    memset(arr.arr, 0, arr.size * sizeof(int)); // Inicializa array a ceros


    do_operations(opt,arr);
    
    print_array(arr);

    free(arr.arr);

    return 0;
}
