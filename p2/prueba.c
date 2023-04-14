#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <string.h>


void MPI_BinomialBcast(void * buf, int count, MPI_Datatype datatype, 
int root,MPI_Comm comm){
    int numprocs, rank;
    MPI_Comm_size(comm, &numprocs);
    MPI_Comm_rank(comm, &rank);

    for(int i = 1; pow(2,i-1) <= numprocs; i++){
        int copy = rank;
        int pot = pow(2,i-1);
        // Si tu rango es menor que 2 elevado al paso actual - 1 entonces 
        //envias si no recibes
        if(rank < pot){ 
            if(rank + pot >= numprocs) break; // Para no multiplos de 2 si te pasas del numero de procesos rompe el bucle
            //printf("Proceso %d en iteracion %d mandando a %d\n",rank,i,rank+pot);
            MPI_Send(&copy,count,datatype,rank+pot,1,comm);
            MPI_Send(&i,1,MPI_INT,rank+pot,1,comm);
        } // && pot < numprocs
        else{
            //printf("Proceso %d en iteracion %d esperando para recibir\n",rank,i);
            MPI_Recv(buf,count,datatype,MPI_ANY_SOURCE,1,comm,NULL);
            MPI_Recv(&i,1,MPI_INT,MPI_ANY_SOURCE,1,comm,NULL);
        }
    }
}




void MPI_Bcast_binomial(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    int rank, size;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    int rounds = (int)(log2(size)) + 1;
    for (int r = 0; r < rounds; r++) {
        int distance = (int)pow(2, r);
        if (rank % distance == 0) {
            int partner = rank + distance;
            if (partner < size) {
                MPI_Send(buffer, count, datatype, partner, 0, comm);
            }
        } else {
            int partner = rank - distance;
            if (partner < 0) {
                continue;
            }
            MPI_Recv(buffer, count, datatype, partner, 0, comm, MPI_STATUS_IGNORE);
            break;
        }
    }
}


int MPI_FlattreeColectiva(void* buff, void* recvbuff, int count, MPI_Datatype datatype,
MPI_Op op, int root, MPI_Comm comm){

    int rank, numprocs, out;
	int *recv;

	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &numprocs);

	out = MPI_SUCCESS;

	if(rank != root) {
		out = MPI_Send(buff, count, datatype, root, 1, comm);
	} else {
		recv = malloc(sizeof(int) * count);
		memcpy(recvbuff, buff, count * sizeof(int));

		for(int i = 0; i < numprocs; i++) {
			if(i == root)
				continue;

			out = MPI_Recv(recv, count, datatype, i, 1, comm, NULL);

			for(int j = 0; j < count; j++) {
				((int *)recvbuff)[j] += recv[j];
			}
		}

		free(recv);
	}

	return out;

}

int main(int argc, char *argv[]){
    MPI_Init(&argc, &argv);

    int a;


    int numprocs, rank, namelen;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); 

    a = 20;
    int b;


    //MPI_FlattreeColectiva(&a,&b,1,MPI_INT,MPI_SUM,0,MPI_COMM_WORLD);

    //if(rank == 0) printf("Hola %d\n",b);
    MPI_BinomialBcast(&b,1,MPI_INT,0,MPI_COMM_WORLD);
    printf("El proceso %d tiene almacenado el numero %d\n",rank,b);
    MPI_Finalize();

    exit(0);
}
