#include <stdio.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 0

#define N 1024

int main(int argc, char *argv[] ) {
  
  int my_id, numprocs;
    
  MPI_Init(&argc, &argv);
            
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_id);
  
  int i, j, remainder = N % numprocs, filasProceso = N/numprocs, countXProcM[numprocs], displsV[numprocs], displsM[numprocs], countXProcV[numprocs], microComp[numprocs], microComu[numprocs], microDispls[numprocs], microCount[numprocs]; //M of matrix and V of vector
  
  if(remainder != 0 && my_id < remainder){
    filasProceso++;
  }
  
  float matrix[N][N];
  float trozoM[filasProceso][N];
  
  float vector[N];
  
  float result[N];
  float trozoR[filasProceso];
  
  struct timeval  tv1, tv2;
  
  if(my_id == 0){
  
    /* Initialize Matrix and Vector */
    for(i=0;i<N;i++) {
      vector[i] = i;
      for(j=0;j<N;j++) {
        matrix[i][j] = i+j;
      }
    }
    
    for(i = 0; i < numprocs; i++){
      
      if(remainder != 0 && i < remainder){
        countXProcM[i] = N*filasProceso;
        countXProcV[i] = filasProceso;
      }else{
        countXProcM[i] = N*(N/numprocs);
        countXProcV[i] = N/numprocs;
      }
      
      if(i == 0){
        displsV[i] = 0;
        displsM[i] = 0;
      }
      else{
        displsV[i] = displsV[i-1] + countXProcV[i-1];
        displsM[i] = displsM[i-1] + countXProcM[i-1];
      }
      microDispls[i] = i;
      microCount[i] = 1;
    }
  }
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  gettimeofday(&tv1, NULL);
  
  MPI_Scatterv(&matrix, countXProcM, displsM, MPI_FLOAT, &trozoM, N*filasProceso, MPI_FLOAT, 0, MPI_COMM_WORLD);
  
  MPI_Bcast(&vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);
  
  gettimeofday(&tv2, NULL);
  int microsecondsComunicacion = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  gettimeofday(&tv1, NULL);

  for(i=0;i<filasProceso;i++) {
    trozoR[i]=0;
    for(j=0;j<N;j++) {
      trozoR[i] += trozoM[i][j]*vector[j];
    }
  }

  gettimeofday(&tv2, NULL);
  int microsecondsComputacion = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
  
  MPI_Barrier(MPI_COMM_WORLD);
  
  gettimeofday(&tv1, NULL);
  
  MPI_Gatherv(&trozoR, filasProceso, MPI_FLOAT, &result, countXProcV, displsV, MPI_FLOAT, 0, MPI_COMM_WORLD); //Envío de los cálculos
  
  MPI_Gatherv(&microsecondsComputacion, 1, MPI_INT, &microComp, microCount, microDispls, MPI_INT, 0, MPI_COMM_WORLD); //Envío de los microsecondsComputacion
  
  gettimeofday(&tv2, NULL);
  microsecondsComunicacion += (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);
  
  MPI_Gatherv(&microsecondsComunicacion, 1, MPI_INT, &microComu, microCount, microDispls, MPI_INT, 0, MPI_COMM_WORLD); //Envío de los microsecondsComunicacion
  
  
  if(my_id == 0){
    /*Display result */
    if (DEBUG){
      for(i=0;i<N;i++) {
        printf(" %f \t ",result[i]);
      }
    } else {
      
      for(i = 0; i < numprocs; i++){
        printf ("Computation time of the process %d (seconds) = %lf\n", i, (double) microComp[i]/1E6);
        printf ("Comunication time of the process %d (seconds) = %lf\n", i, (double) microComu[i]/1E6);
      }
    }
  }
  
  MPI_Finalize();
  return 0;
}
