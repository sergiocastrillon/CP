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


int MPI_BinomialBcast(void * buf, int count, MPI_Datatype datatype, 
int root,MPI_Comm comm){
    int numprocs, rank, err;
    MPI_Comm_size(comm, &numprocs);
    MPI_Comm_rank(comm, &rank);

    err = MPI_SUCCESS;

    // Control de errores que no tiene en cuenta el send o recv
    if(root != 0) return MPI_ERR_ROOT; // Solo funciona para proceso 0

    int pot;
    for(int i = 1; (pot = pow(2,i-1)) < numprocs; i++){
        int copy = rank;
        // Si tu rango es menor que 2 elevado al paso actual - 1 entonces 
        //envias si no recibes
        if(rank < pot){ 
            if(rank + pot >= numprocs) break; // Garantizamos que no nos pasamos del numero de procesos
            //printf("Proceso %d en iteracion %d mandando a %d\n",rank,i,rank+pot);
            err = MPI_Send(buf,count,datatype,rank+pot,1,comm);
            if(err != MPI_SUCCESS) return err;
            MPI_Send(&i,1,MPI_INT,rank+pot,1,comm); // Sincronizacion del paso
        }else{
            //printf("Proceso %d en iteracion %d esperando para recibir\n",rank,i);
            err = MPI_Recv(buf,count,datatype,MPI_ANY_SOURCE,1,comm,NULL);
            MPI_Recv(&i,1,MPI_INT,MPI_ANY_SOURCE,1,comm,NULL); // Sincronizacion del paso
            if(err != MPI_SUCCESS) return err;
        }
    }
    return err; // Se espera que sea SUCCESS
}



int MPI_FlattreeColectiva(void* buf, void* recvbuff, int count, MPI_Datatype datatype,
MPI_Op op, int root, MPI_Comm comm){

    int rank, numprocs, err = MPI_SUCCESS;
	int *recv;

	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &numprocs);

    // Control de errores que no tiene en cuenta el send o recv
    if(op != MPI_SUM) return MPI_ERR_OP;
    if(datatype != MPI_INT) return MPI_ERR_TYPE;
    

	if(rank != root) {
		err = MPI_Send(buf, count, datatype, root, 404, comm);
	} else {
		recv = malloc(sizeof(int) * count);
		memcpy(recvbuff, buf, count * sizeof(int));

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

    // Realmente el if no hace falta pero garantiza que solo el 0 recibe los argumentos
    // y que por tanto la función envía y recibe bien
    if(rank == 0){
        n = atoi(argv[1]);
        L = *argv[2];
    }
    
    MPI_BinomialBcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_BinomialBcast(&L,1,MPI_CHAR,0,MPI_COMM_WORLD);
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