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

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1); 
    }
    
    
    

    int i, n, count=0;
    char *cadena;
    char L;

    n = atoi(argv[1]);
    L = *argv[2];
    

    MPI_Init(&argc, &argv);

    int numprocs, rank, namelen;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 

    if(rank == 0){
        for(int i = 1; i < numprocs; i++) MPI_Send(&n,1,MPI_INT,i,
        1,MPI_COMM_WORLD); // 1 es el tag
        for(int i = 1; i < numprocs; i++) MPI_Send(&L,1,MPI_CHAR,i,
        1,MPI_COMM_WORLD);
    }

    if(rank != 0){
        int n;
        char L;

        MPI_Recv(&n,1,MPI_INT,0,1,MPI_COMM_WORLD,NULL);
        MPI_Recv(&L,1,MPI_CHAR,0,1,MPI_COMM_WORLD,NULL);
    }
    
    //printf("Proceso %d tiene %d y %c\n",rank,n,L);
    
    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);

    for(i=rank; i<n; i+=numprocs){
        if(cadena[i] == L){
        count++;
        }
    }

    if(rank == 0){
        int aux;
        for(int i = 1; i < numprocs; i++){
            MPI_Recv(&aux,1,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,NULL);
            count+= aux;
        }
    }

    if(rank != 0){
        MPI_Send(&count,1,MPI_INT,0,1,MPI_COMM_WORLD); // 1 es el tag
    }

    free(cadena);

    MPI_Finalize();

    if(rank != 0) exit(0);

    printf("El numero de apariciones de la letra %c es %d\n", L, count);

    exit(0);
}