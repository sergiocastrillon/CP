#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 1

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  100 // Number of sequences
#define N  200  // Number of bases per sequence

unsigned int g_seed = 0;

int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5;
}

// The distance between two bases
int base_distance(int base1, int base2){

  if((base1 == 4) || (base2 == 4)){
    return 3;
  }

  if(base1 == base2) {
    return 0;
  }

  if((base1 == 0) && (base2 == 3)) {
    return 1;
  }

  if((base2 == 0) && (base1 == 3)) {
    return 1;
  }

  if((base1 == 1) && (base2 == 2)) {
    return 1;
  }

  if((base2 == 2) && (base1 == 1)) {
    return 1;
  }

  return 2;
}

int main(int argc, char *argv[] ) {

  int i, j;
  int *data1, *data2;
  int *result;
  struct timeval  tv1, tv2;
  

  MPI_Init(&argc,&argv);

  int *datar1, *datar2;

  int numprocs, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  

  if(rank == 0){

    data1 = (int *) malloc(M*N*sizeof(int));
    data2 = (int *) malloc(M*N*sizeof(int));
    result = (int *) malloc(M*sizeof(int));

    /* Initialize Matrices */
    for(i=0;i<M;i++) { // i indica la columna y j la fila
      for(j=0;j<N;j++) {
        /* random with 20% gap proportion */
        data1[i*N+j] = fast_rand();
        data2[i*N+j] = fast_rand();
      }
    }
  }
  


  

  int* sendcounts = malloc(numprocs * sizeof(int));
  int* recvcounts = malloc(numprocs * sizeof(int));

  int* displs = malloc(numprocs * sizeof(int));
  int* rdispls = malloc(numprocs * sizeof(int));

  int filas_p_proc = M / numprocs; // Número de filas por proceso
  int resto = M % numprocs; // Filas sobrantes

  int recvcount;
  int* resultr = (int *) malloc((filas_p_proc)*sizeof(int));


  if(rank < resto){
    recvcount = (filas_p_proc + 1)*N;
    datar1 = (int *) malloc(recvcount*sizeof(int));
    datar2 = (int *) malloc(recvcount*sizeof(int));
    
  }else{
    recvcount = filas_p_proc*N;
    datar1 = (int *) malloc((recvcount)*sizeof(int));
    datar2 = (int *) malloc((recvcount)*sizeof(int));
  }
  

  for (int i = 0; i < numprocs; i++) {
    sendcounts[i] = filas_p_proc*N;
    recvcounts[i] = filas_p_proc;

    if (i < resto){
      sendcounts[i] = sendcounts[i] + N;
      recvcounts[i] = recvcounts[i] + 1;
    } 
    // displs indica el desplazamiento en elementos, es decir si displs[1] = 2 y arr
    // tiene 4 elementos quiere decir que el proceso 1 recibirá arr[2] y arr[3];

    if(i > 0){
      displs[i] = displs[i-1] + sendcounts[i-1];
      rdispls[i] = rdispls[i-1] + recvcounts[i-1];
    } 
    else{
      displs[i] = 0;
      rdispls[i] = 0;
    } 
    // Si el proceso es el 0 empiezas en el principio si no coges el desplazamiento del
    // anterior y se lo sumas a donde empezó ese proceso anterior 
  }


  printf("Proceso %d tiene rdispls = %d\n",rank,rdispls[rank]);
  MPI_Scatterv(data1,sendcounts,displs,MPI_INT,datar1,recvcount,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Scatterv(data2,sendcounts,displs,MPI_INT,datar2,recvcount,MPI_INT,0,MPI_COMM_WORLD);


  gettimeofday(&tv1, NULL);

  for(i=0;i<(recvcounts[rank]);i++) {
    resultr[i]=0;
    for(j=0;j<N;j++) {
      resultr[i] += base_distance(datar1[i*N+j], datar2[i*N+j]);
    }
    printf("%d en fila %d proceso %d\n",resultr[i],i,rank);
  }

  gettimeofday(&tv2, NULL);
    
  int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  MPI_Gatherv(resultr,recvcounts[rank],MPI_INT,result,recvcounts,rdispls,MPI_INT,0,MPI_COMM_WORLD);

  if(rank == 0){
    /* Display result */
    if (DEBUG == 1) {
      int checksum = 0;
      for(i=0;i<M;i++) {
        checksum += result[i];
      }
      printf("Checksum: %d\n ", checksum);
    } else if (DEBUG == 2) {
      for(i=0;i<M;i++) {
        printf(" %d \t ",result[i]);
      }
    } else {
      printf ("Time (seconds) = %lf\n", (double) microseconds/1E6);
    }    
  }
  

  free(data1); free(data2); free(result);

  MPI_Finalize();

  return 0;
}

