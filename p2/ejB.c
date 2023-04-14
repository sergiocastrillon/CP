#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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


int MPI_BinomialBcast(void * buff, int count, MPI_Datatype datatype, 
int root,MPI_Comm comm){
    int numprocs, rank, err = MPI_SUCCESS;
    MPI_Comm_size(comm, &numprocs);
    MPI_Comm_rank(comm, &rank);
    
    for(int i = 1; pow(2,i-1) <= numprocs; i++){
        int pot = pow(2,i-1);
        // Si tu rango es menor que 2 elevado al paso actual - 1 entonces 
        //envias si no recibes
        if(rank < pot){ 
            // Para no multiplos de 2 si te pasas del numero de procesos rompe el bucle
            if(rank + pot >= numprocs) break;
            err = MPI_Send(&buff,count,datatype,rank+pot,404,comm);
            MPI_Send(&i,1,MPI_INT,rank+pot,1,comm); // Los posibles fallos de esta función también
            // se darían en la anterior así que no hace falta comprobarlo
            if(err != MPI_SUCCESS) return err;
        }else{
            err = MPI_Recv(buff,count,datatype,MPI_ANY_SOURCE,404,comm,NULL);
            MPI_Recv(&i,1,MPI_INT,MPI_ANY_SOURCE,404,comm,NULL);
            if(err != MPI_SUCCESS) return err;
        }
    }
    return err; // Supuestamente debería ser MPI_SUCCESS
}

int MPI_FlattreeColectiva(void* buff, void* recvbuff, int count, MPI_Datatype datatype,
MPI_Op op, int root, MPI_Comm comm){

    int rank, numprocs, err = MPI_SUCCESS;
	int *recv;

	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &numprocs);

    // if() falta el control de errores que no hace el send o recv

	if(rank != root) {
		err = MPI_Send(buff, count, datatype, root, 404, comm);
	} else {
		recv = malloc(sizeof(int) * count);
		memcpy(recvbuff, buff, count * sizeof(int));

		for(int i = 0; i < numprocs - 1; i++) {
			err = MPI_Recv(recv, count, datatype, MPI_ANY_SOURCE, 404, comm, NULL);
            if(err != MPI_SUCCESS) return err;
			for(int j = 0; j < count; j++) {
				((int *)recvbuff)[j] += recv[j];
			}
		}
        free(recv);
	}

	return err; // Puede devolver un error si viene del MPI_Send

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
    MPI_FlattreeColectiva(&count,&count_gen,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);
    
    free(cadena);

    MPI_Finalize();

    if(rank == 0) printf("El numero de apariciones de la letra %c es %d\n", L, count_gen);

    exit(0);
}