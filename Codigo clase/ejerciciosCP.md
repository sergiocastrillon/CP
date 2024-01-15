2. Puente para coches

mtx_t bridge;
int cars[2];
cnt_t bridge_ocuppied[2];
// Apartados A y B
void enter_bridge(int direction){
    lock(bridge);
    while(cars[dir+1]&2)] > 0 || cars[dir] == max_cars) // Segunda condicion    para no superar capacidad del puente
        wait(bridge_ocuppied[dir],bridge);
    cars[dir]++;
    unlock(bridge);
}


void exit_bridge(int dir){
    lock(bridge);
    cars[dir]--;
    if(cars[dir]==0) broadcast(bridge_ocuppied[(dir+1)%2]);
    if(cars[dir]== max_cars - 1) broadcast(bridge_ocuppied[dir]);
    unlock(bridge)
}


// Con semaforos
SEM_T capacity = MAX_CARS

void enter_bridge(int direction){
    lock(bridge);
    while(cars[dir+1]&2)] > 0) // Segunda condicion    para no superar capacidad del puente
        wait(bridge_ocuppied[dir],bridge);
    cars[dir]++;
    unlock(bridge);
    P(capacity)
}


void exit_bridge(int dir){
    V(capacity)
    lock(bridge);
    cars[dir]--;
    if(cars[dir]==0) broadcast(bridge_ocuppied[(dir+1)%2]);
    unlock(bridge)
}



2. Museo
struct group{
    const int members; // miembros totales en el grupo
    phread_mutex_t *counter_s; // mutex para proteger el contador
    int counter; // numero de miembros dentro el museo
}
int capacity;

void *visitor(void *arg){
    struct group *grp = arg;
    // Access

    visit();

    //Exit
}

////////////////////////////
enter(struct group *grp){
    lock(grp -> counter_m);
    if(grp->counter==0){ //primer
        lock(capacity_m);
        while(capacity < grp->members) wait(no_capacity, capacity_m);
        capacity -= grp->members;
        unlock(capacity_m);
    }
    grp->counter++;
    unlock(grp->counter_m);
}

exit(struct group *grp){
    lock(grp->counter_m);
    grp->counter--;
    if(grp->counter=0){
        lock(capacity_m);
        capacity+=(grp->members);
        broadcast(no_capacity); // Puede que despertemos a un grupo que es mas grande que el nuestro y no pueda entrar, entonces mejor despertar a todos los grupos para que si hay uno de igual o mayor capacidad pueda entrar ese.

        unlock(capacity_m);
    }
    unlock(grp->counter_m);
}



// Solucionar que alguien se vaya antes de que entre el grupo




////////////////////////////
enter(struct group *grp){
    lock(grp -> counter_m);
    if(grp->counter==0){ //primer
        lock(capacity_m);
        while(capacity < grp->members) wait(no_capacity, capacity_m);
        capacity -= grp->members;
        unlock(capacity_m);
    }
    grp->counter++;
    if(grp->counter < grp->members) wait(grp->wait_for_all,grp->counter_m)// no ultimo
    else broadcast(grp->wait_for_all);
    unlock(grp->counter_m)
}

exit(struct group *grp){
    lock(grp->counter_m);
    grp->counter--;
    if(grp->counter=0){
        lock(capacity_m);
        capacity+=(grp->members);
        broadcast(no_capacity); // Puede que despertemos a un grupo que es mas grande que el nuestro y no pueda entrar, entonces mejor despertar a todos los grupos para que si hay uno de igual o mayor capacidad pueda entrar ese.

        unlock(capacity_m);
    }
    unlock(grp->counter_m);
}


3. Futures

struct future{
    void* (*f)(void*);
    void *arg;
    thrd_t thr;
    bool done;
    mtx_t done_m;
    cnd_t not_done;
};

struct future *promise(void*(*f)(void*),void *arg){
    struct future *fut = malloc(sizeof(struct future));
    fut->f=f; fut->arg = arg;
    mtx_init(&fut->done_m,mtx_plain);
    fut->done = false;
    cnd_init(&fut->not_done);


    thrd_create(&fut->thr,do_f);

    return fut;


}

int do_f(void *arg){
    struct future *fut = arg;
    void *res = fut->f(fut->arg);
    lock(fut->done_m);
    fut->done = true;
    fut->res = res;
    broadcast(fut->not_done); // puede haber mas de uno esperando por el resultado
    unlock(fut->done_m);
}

void* force(struct future *fut){
    lock(fut->done_m);
    if(!fut->done) wait(fut->not_done, fut->done_m);
    unlock(fut->done_m);
    return res;
}


free_future(struct future *fut){
    cnd_destroy(fut->not_done);
    mtx_destroy(fut->done_m);
    thrd_join(fut->thrd,NULL);
    free(fut);
}
