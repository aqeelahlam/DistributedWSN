#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

#define NUM_THREADS 1
#define SATELLITE_SIZE 100
#define MAX_TEMP_RANGE 150
#define MIN_TEMP_RANGE 80


//void b_station()
void *ThreadFunc(void *pArg);


//struct sThreadRes {
//    int *m_temps;
//    };

struct satValues{
    int x_coord;
    int y_coord;
    int timestamp;
    int temp;
    };
    
struct satValues *satelliteTemps[100];
int satelliteTempsCount;



void *ThreadFunc(void *pArg){
  
    //int *pData = (int*)pArg;
    //int my_rank = *pData;
    //struct sThreadRes *pThreadRes = (struct sThreadRes*)malloc(sizeof(struct sThreadRes));
    
    //pThreadRes->m_temps = (int*)malloc((??????) * sizeof(int));
    
    unsigned int seed = time(NULL) * satelliteTempsCount;
	    
		/*scaling the output of rand_r() to be in between MIN_RANGE and MAX_RANGE
		 and assigning it to a position in the list */
	satelliteTemps[satelliteTempsCount]->temp = rand_r(&seed) % (MAX_TEMP_RANGE + 1 - MIN_TEMP_RANGE) + MIN_TEMP_RANGE;
	satelliteTempsCount++;

    
    return NULL;
    }


int main(){
   
    *satelliteTemps = (struct satValues*)malloc(SATELLITE_SIZE * sizeof(struct satValues));
    satelliteTempsCount = 0;
    
    pthread_t tid[NUM_THREADS];
    int threadNum[NUM_THREADS];
    MPI_Status status;
    //struct sThreadRes *pThreadRes = NULL; 
   
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
	    	//pthread_join(tid[i], (void**)&pThreadRes);
	    	pthread_join(tid[i], NULL);
	}

    
    // wait for messages from WSN nodes
    //while(!flag){
    // fixed loop iterates 100 times
    for (int i = 0; i<100; i++){
        MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
        if (flag) {
            MPI_Recv(&recvMsg, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
            // Compare recvMsg with thread temp
            
           }
    }
    
    //MPI_Wait(&request, &status);
    return 0;
}
