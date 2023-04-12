#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

void inicializaCadena(char *cadena, int n){
    int i;
    for(i=0; i<n/2; i++){
        cadena[i] = 'A';
    }
    for(i=n/2; i<3*n/4; i++){
        cadena[i] = 'C';
    }
    for(i=3*n/4; i<9*n/10; i++){
        cadena[i] = 'G';
    }
    for(i=9*n/10; i<n; i++){
        cadena[i] = 'T';
    }
}


void MPI_BinomialBcast(void * buf, int count, MPI_Datatype datatype, 
int root,MPI_Comm comm){
    int numprocs, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for(int i = 0; pow(2,i) < numprocs; i++){
        if(rank < pow(2,i-1) && pow(2,i-1) < numprocs) 
        MPI_Send(buf,count,datatype,rank+pow(2,i-1),MPI_ANY_TAG,comm);
        
    }
}

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1); 
    }
    
    MPI_Init(&argc, &argv);

    int i, n, count=0;
    char *cadena;
    char L;


    int numprocs, rank, namelen;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 

    if(rank == 0){
        n = atoi(argv[1]);
        L = *argv[2];
    }
    
    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&L,1,MPI_CHAR,0,MPI_COMM_WORLD);
    //printf("Proceso %d tiene %d y %c\n",rank,n,L);
    
    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);

    for(i=rank; i<n; i+=numprocs){
        if(cadena[i] == L){
        count++;
        }
    }


    int count_gen;
    MPI_Reduce(&count,&count_gen,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
    
    free(cadena);

    MPI_Finalize();

    if(rank == 0) printf("El numero de apariciones de la letra %c es %d\n", L, count_gen);

    exit(0);
}