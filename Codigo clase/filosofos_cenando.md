--Filosofos cenando--
// Si hay n filosofos hay n tenedores o n elementos
N;
mtx_t fork[N];
#define RF(I) (I) // El tenedor de la derecha es el tenedor I
#define LF(I) ((I+1)%N) // El de la izq es el tenedor I+1 mod el numero de filosofos

// Posible solucion con menor y mayor (previene el interbloqueo pero es lento porque se soluciona de forma ciclica el interbloqueo)
#define MIN(X,Y) ((X)<(Y)? (X):(Y))
#define MAX(X,Y) ((X)>=(Y)? (X):(Y))

porque se soluciona de forma ciclica el interbloqueo)
pickup(int I){ // I es numero de filosofo
    lock(fork[MIN(RF(I),LF(I))]);
    lock(fork[MAX(RF(I),LF(I))]);
}

put_down(int I){
    unlock(fork[MIN(RF(I),LF(I))]);
    unlock(fork[MAX(RF(I),LF(I))]);
}

// Posible solucion con trylocks (la posibilidad de que solo uno coma se reduce por lo que para esta solucion particular es mejor)
while(true){
    lock(fork[RF(I)]);
    if(trylock(fork[LF(I)])==0) break; // 0 si se ha bloqueado con exito
    unlock(fork[RF(I)]);
    usleep(randr()%10); // Esperan un tiempo distinto para que no vuelvan a coincidir en llegar y vuelvan a soltar los tenedores
}



// Representacion del estado de la mesa/filosofos en vez de los tenedores

mtx_t table;
cnd_t forks_in_use[N];
#define LP(I) (I) // Left philosopher
#define RP(I) (I==0 ? N-1: I-1)

int PH[N];
#define EATING 0
#define THINKING 1
// Solución para saber si alguien espera
#define HUNGRY 2

pickup(int i){
    lock(table);
    PH[I] = HUNGRY;
    while(PH[RP(I)]==EATING) || PH[LP(I)] == EATING) wait(forks_in_use[I],table);
    PH[I] = EATING;
    unlock(table);
}

put_down(int I){
    lock(table);
    PH[I] = THINKING;
    if(PH[RP(I)]==HUNGRY) signal(forks_in_use[RP(I)]);
    if(PH[LP(I)]==HUNGRY) signal(forks_in_use[LP(I)]);

    unlock(table);
}


