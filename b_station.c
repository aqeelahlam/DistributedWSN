#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

void b_station(){
    int flag = 0;
    int recvMsg = 0;
    // wait for messages from WSN nodes
    while(!flag){
        MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            MPI_Recv(&recvMsg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status );
            // Compare recvMsg with thread temp
           }
    }
}
