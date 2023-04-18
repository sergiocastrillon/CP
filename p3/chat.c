int* sendcounts = malloc(procs * sizeof(int));
int* displs = malloc(procs * sizeof(int));
int elements_per_proc = m * n / procs; // Número de elementos por proceso
int remainder = m * n % procs; // Elementos sobrantes

for (int i = 0; i < procs; i++) {
    sendcounts[i] = elements_per_proc;
    if (i < remainder) {
        sendcounts[i]++;
    }
    // displs indica el desplazamiento en elementos, es decir si displs[1] = 2 y arr
    // tiene 4 elementos quiere decir que el proceso 1 recibirá arr[2] y arr[3];
    displs[i] = (i > 0) ? (displs[i-1] + sendcounts[i-1]) : 0;
}
