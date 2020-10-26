#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

#define NUM_THREADS 1


void b_station()
void *ThreadFunc(void *pArg);

struct sThreadRes {
    int *m_temps;
    };


void *ThreadFunc(void *pArg){
    int i;
    int *pData = (int*)pArg;
    int my_rank = *pData;
    struct sThreadRes *pThreadRes = (struct sThreadRes*)malloc(sizeof(struct sThreadRes));
    
    //first = (my_rank) * gUpperLimit/NUM_THREADS + 1;
    //last = (my_rank +1) * gUpperLimit/NUM_THREADS;
    
    pThreadRes->m_temp = ;
    
    return pThreadRes;
    }


void b_station(){
    pthread_t tid[NUM_THREADS];
    int threadNum[NUM_THREADS];
    struct sThreadRes *pThreadRes = NULL; 
   
    int flag = 0;
    int recvMsg = 0;
    
 
    // Fork	
    // Create the threads	
	for (int i = 0; i < NUM_THREADS; i++)
	{
	    threadNum[i] = i;
		pthread_create(&tid[i], 0, ThreadFunc, &threadNum[i]);
	}
	
	// Join
	// Wait for all threads to finish
	for(int i = 0; i < NUM_THREADS; i++)
	{
	    	pthread_join(tid[i], (void**)&pThreadRes);
	}

    
    // wait for messages from WSN nodes
    //while(!flag){
    // fixed loop iterates 100 times
    for (int i = 0; i<100; i++){
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            MPI_Recv(&recvMsg, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
            // Compare recvMsg with thread temp
            // ... here...
           }
    }
    
    //MPI_Wait(&request, &status);
}
