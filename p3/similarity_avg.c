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

#define M  1000000 // Number of sequences
#define N  200

//#define M 10
//#define N 3

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

  int sendcount;

  


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
  


  

  


  if(rank < resto) sendcount = (filas_p_proc + 1)*N;
  else sendcount = filas_p_proc*N;

  if(rank != 0){ // Si rango no es 0 entonces reservas del tamaño correspondiente
    data1 = (int *) malloc((sendcount)*sizeof(int));
    data2 = (int *) malloc((sendcount)*sizeof(int));
    result = (int *) malloc((sendcount/N)*sizeof(int));

    // Inicializamos recvcounts[rank] en cada proceso para el Gatherv
    recvcounts[rank] = filas_p_proc;
    if(rank < resto) recvcounts[rank] = recvcounts[rank] + 1;

  }else{ 
    // Solo el 0 necesita estos datos asi que ahorramos computación el el resto de procesos

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

  }
  

  

  // Recibimos las filas con las que operar
  gettimeofday(&tv1, NULL);

  MPI_Scatterv(data1,sendcounts,displs,MPI_INT,data1,sendcount,MPI_INT,0,MPI_COMM_WORLD);
  MPI_Scatterv(data2,sendcounts,displs,MPI_INT,data2,sendcount,MPI_INT,0,MPI_COMM_WORLD);

  gettimeofday(&tv2, NULL);

  int commTime = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  gettimeofday(&tv1, NULL);

  for(i=0;i<(recvcounts[rank]);i++) {
    result[i]=0;
    for(j=0;j<N;j++) {
      result[i] += base_distance(data1[i*N+j], data2[i*N+j]);
    }
  }

  gettimeofday(&tv2, NULL);
    
  int procTime = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

  gettimeofday(&tv1, NULL);
  // Devolvemos los resultados
  MPI_Gatherv(result,recvcounts[rank],MPI_INT,result,recvcounts,gatherdispls,MPI_INT,0,MPI_COMM_WORLD);
  gettimeofday(&tv2, NULL);

  commTime = commTime + (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
  
  int procAvg, commAvg;
  MPI_Reduce(&procTime, &procAvg, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&commTime, &commAvg, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


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
    }else{
      procAvg = procAvg / numprocs; commAvg = commAvg / numprocs;
      printf("\n--\nAverage computing time: %lf (seconds)\n", i, (double) procAvg/1E6);
      printf("Average communication time: %lf (seconds)\n",(double) commAvg/1E6);
      printf("Total average time: %lf (seconds)\n--\n", (double) ((procAvg + commAvg)/2/1E6));
    }
  }


  free(data1); 
  free(data2); 
  free(result);
  free(sendcounts);
  free(recvcounts);

  MPI_Finalize();
  return 0;
}
  
  
  
