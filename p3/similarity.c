#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 0

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  10000 // Number of sequences
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


  MPI_Init(&argc,&argv);

  int i, j;
  int *data1, *data2;
  int *result;
  struct timeval  tv1, tv2;
  
  int *datar1, *datar2;

  int numprocs, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int* sendcounts = malloc(numprocs * sizeof(int));
  int* recvcounts = malloc(numprocs * sizeof(int));


  // displs indica el desplazamiento en elementos, es decir si displs[1] = 2 y arr
  // tiene 4 elementos quiere decir que el proceso 1 recibirá arr[2] y arr[3];
  int* displs = malloc(numprocs * sizeof(int));
  int* gatherdispls = malloc(numprocs * sizeof(int));

  int filas_p_proc = M / numprocs; // Número de filas por proceso
  int resto = M % numprocs; // Filas sobrantes

  int recvcount;
  
  int* proresult = (int *) malloc((filas_p_proc)*sizeof(int)); // Resultado individual de cada proceso

  data1 = (int *) malloc(M*N*sizeof(int));
  data2 = (int *) malloc(M*N*sizeof(int));
  result = (int *) malloc(M*sizeof(int));


  if(rank == 0){
    /* Initialize Matrices */
    for(i=0;i<M;i++) { // i indica la columna y j la fila
      for(j=0;j<N;j++) {
        /* random with 20% gap proportion */
        data1[i*N+j] = fast_rand();
        data2[i*N+j] = fast_rand();
      }
    }
  }
  


  

  


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
    // Cada proceso recibe filas_p_proc * N elementos y debe devolver un elemento por fila
    sendcounts[i] = filas_p_proc*N;
    recvcounts[i] = filas_p_proc;

    if (i < resto){ // Mandamos el resto a los primeros "resto" procesos
      sendcounts[i] = sendcounts[i] + N;
      recvcounts[i] = recvcounts[i] + 1;
    } 
    
    // displs se calcula en base a sendcounts
    if(i > 0){
      displs[i] = displs[i-1] + sendcounts[i-1];
      gatherdispls[i] = gatherdispls[i-1] + recvcounts[i-1];
    } 
    else{
      // El proceso 0 recibe los primeros elementos
      displs[i] = 0;
      gatherdispls[i] = 0;
    } 
    // Si el proceso es el 0 empiezas en el principio si no coges el desplazamiento del
    // anterior y se lo sumas a donde empezó ese proceso anterior 
  }

  // Recibimos las filas con las que operar
  gettimeofday(&tv1, NULL);

  MPI_Scatterv(data1,sendcounts,displs,MPI_INT,datar1,recvcount,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Scatterv(data2,sendcounts,displs,MPI_INT,datar2,recvcount,MPI_INT,0,MPI_COMM_WORLD);

  gettimeofday(&tv2, NULL);

  int commTime = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  gettimeofday(&tv1, NULL);

  for(i=0;i<(recvcounts[rank]);i++) {
    proresult[i]=0;
    for(j=0;j<N;j++) {
      proresult[i] += base_distance(datar1[i*N+j], datar2[i*N+j]);
    }
  }

  gettimeofday(&tv2, NULL);
    
  int procTime = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  gettimeofday(&tv1, NULL);
  // Devolvemos los resultados
  MPI_Gatherv(proresult,recvcounts[rank],MPI_INT,result,recvcounts,gatherdispls,MPI_INT,0,MPI_COMM_WORLD);
  gettimeofday(&tv2, NULL);

  commTime = commTime + (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
  

  if(DEBUG != 1 && DEBUG != 2){
    printf ("Processing time of process %d (seconds) = %lf\n", rank, (double) procTime/1E6);
    printf("Communication time of process %d (seconds) = %lf\n", rank, (double) commTime/1E6);
  }

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
    }
  }


  free(data1); 
  free(data2); 
  free(result);
  free(sendcounts);
  free(recvcounts);
  free(datar1);
  free(datar2);

  MPI_Finalize();
  return 0;
}
  
  
  