#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>

#define MSG_RESPOND_ALIVE 2

#define SLEEP_MICRO_SEC	0000000
#define wait_limit_sec		2
#define TERMINATION_FAULT 10 

int master_io(MPI_Comm world_comm, MPI_Comm comm);
int slave_io(MPI_Comm world_comm, MPI_Comm comm);
void* ProcessFunc(void *pArg);

pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;
int g_nslaves = 0;	
int activity_check = 1;


// C-main function
int main(int argc, char **argv)
{
    int rank, size;
    MPI_Comm new_comm;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    MPI_Comm_split( MPI_COMM_WORLD,rank == size-1, 0, &new_comm);
    if (rank == size-1) 
	master_io( MPI_COMM_WORLD, new_comm );
    else
	slave_io( MPI_COMM_WORLD, new_comm );
    MPI_Finalize();
    return 0;
}

// POSIX thread function
void* ProcessFunc(void *pArg)
{
	char buf[256];
	MPI_Status status;
	MPI_Status probe_status;                                
	int flag = 0;
	struct timespec clock_time;       
	struct timespec current_time;                                                                                                                                        
	double elapsed_time; 
	int i;
	int size;
	
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	
	memset(buf, 0, 256 * sizeof(char));
	printf("Thread started\n");
	
	clock_gettime(CLOCK_MONOTONIC, &clock_time); 

	pthread_mutex_lock(&g_Mutex);
		// Saves the current clock time for each slave process
		// The clock time is used to calculate the last active time of each slave process.
		struct timespec *pSlaves_last_alive_timestamp_array = (struct timespec*)malloc(g_nslaves * sizeof(struct timespec)); // dynamic or heap array
		for(i = 0; i < g_nslaves; i++){
			pSlaves_last_alive_timestamp_array[i].tv_sec =  clock_time.tv_sec;
			pSlaves_last_alive_timestamp_array[i].tv_nsec =  clock_time.tv_nsec;
		}
		
		// Saves the last active time of each slave process (i.e., how many seconds a slave process was last active).
		 // This array is not really used in this sample code.
		double *pSlaves_last_alive_elapsed_time_array = (double*)malloc(g_nslaves * sizeof(double)); // dynamic or heap array
		memset(pSlaves_last_alive_elapsed_time_array, 0.0, g_nslaves * sizeof(double));
	pthread_mutex_unlock(&g_Mutex);
	
	int localSlavesCount = 0;
	while (activity_check == 1) {
		pthread_mutex_lock(&g_Mutex);
		if(g_nslaves <= 0){
			pthread_mutex_unlock(&g_Mutex);
			break;
		}
		pthread_mutex_unlock(&g_Mutex);
		
		MPI_Iprobe(MPI_ANY_SOURCE, MSG_RESPOND_ALIVE, MPI_COMM_WORLD, &flag, &probe_status);
		if(flag){
			MPI_Recv(buf, 256, MPI_CHAR, probe_status.MPI_SOURCE, MSG_RESPOND_ALIVE, MPI_COMM_WORLD, &status);
			
			clock_gettime(CLOCK_MONOTONIC, &clock_time); // get the current timestamp
			pSlaves_last_alive_timestamp_array[probe_status.MPI_SOURCE].tv_sec =  clock_time.tv_sec;
			pSlaves_last_alive_timestamp_array[probe_status.MPI_SOURCE].tv_nsec =  clock_time.tv_nsec;
			memset(buf, 0, 256 * sizeof(char));
		}
		pthread_mutex_lock(&g_Mutex);
			localSlavesCount = g_nslaves;
		pthread_mutex_unlock(&g_Mutex);
		
		clock_gettime(CLOCK_MONOTONIC, &current_time);
			for(i = 0; i < localSlavesCount; i++){
				elapsed_time = (current_time.tv_sec - pSlaves_last_alive_timestamp_array[i].tv_sec) * 1e9; 
	    			elapsed_time = (elapsed_time + (current_time.tv_nsec - pSlaves_last_alive_timestamp_array[i].tv_nsec)) * 1e-9; 
				pSlaves_last_alive_elapsed_time_array[i] = elapsed_time;
				printf("Thread prints: Slave - %d. Last active: %lf seconds ago\n", i, elapsed_time);
				fflush(stdout);
				if(elapsed_time >= wait_limit_sec){
					// printf("Thread prints: Slave - %d. Last active: %lf seconds ago,\nSlave - %d is not responding. Process terminated\n", i, elapsed_time, i);
					for(int j = 0; j< size-1; j++){
						MPI_Send(0,0, MPI_INT, j, TERMINATION_FAULT, MPI_COMM_WORLD);
					}
					pthread_mutex_lock(&g_Mutex);
					activity_check = 0;
					pthread_mutex_unlock(&g_Mutex);
					break;
					return 0;
				}
				
			}
		usleep(SLEEP_MICRO_SEC);
	}
	printf("Thread finished\n");
	fflush(stdout);
	free(pSlaves_last_alive_timestamp_array);
	free(pSlaves_last_alive_elapsed_time_array);
	return 0;
}

// Master function
int master_io(MPI_Comm world_comm, MPI_Comm comm)
{
	int size;
	MPI_Comm_size(world_comm, &size );
	g_nslaves = size - 1;
	
	pthread_t tid;
	pthread_mutex_init(&g_Mutex, NULL);
	pthread_create(&tid, 0, ProcessFunc, NULL); // Create the thread

	char buf[256];
	MPI_Status status;
	MPI_Status probe_status;
	int flag = 0;
	int cmp_result = 0;
	
	memset(buf, 0, 256 * sizeof(char));
	printf("MPI Master Process started\n");
	while (1) {
		pthread_mutex_lock(&g_Mutex);
		if(activity_check == 0){
			break;
		}
		pthread_mutex_unlock(&g_Mutex);
		
		
		
		usleep(SLEEP_MICRO_SEC);
	}
	printf("MPI Master Process finished\n");
	fflush(stdout);
	
	pthread_join(tid, NULL);
	pthread_mutex_destroy(&g_Mutex);
	
    return 0;
}

// Slave (or worker) function
int slave_io(MPI_Comm world_comm, MPI_Comm comm)
{
	int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, masterSize;
	MPI_Comm comm2D;
	int dims[ndims],coord[ndims];
	int wrap_around[ndims];
	char buf[256];
	int alive_count = 0;

    
    	memset(buf, 0, 256 * sizeof(char));
    	MPI_Comm_size(world_comm, &masterSize); // size of the master communicator
  	MPI_Comm_size(comm, &size); // size of the slave communicator
	MPI_Comm_rank(comm, &my_rank);  // rank of the slave communicator
	dims[0]=dims[1]=0;
	
	MPI_Dims_create(size, ndims, dims);
    	if(my_rank==0)
		printf("Slave Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",my_rank,size,dims[0],dims[1]);

    	/* create cartesian mapping */
	wrap_around[0] = 0;
	wrap_around[1] = 0; /* periodic shift is .false. */
	reorder = 0;
	ierr =0;
	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);
	
	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord); // coordinated is returned into the coord array
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);

	
	MPI_Status fault_status; int fault_flag;
	
	int counter=0;
	
	while(1){
	
		// To detect incoming message from base station (for the fault detected)
		MPI_Iprobe(masterSize-1, TERMINATION_FAULT, world_comm, &fault_flag, &fault_status);
		if(fault_flag){
			printf("[Process %d] Fault detected\n", my_rank);
			break;
		}

		// To notify base station that this is active
		sprintf( buf, "Slave %d at Coordinate: (%d, %d). ALIVE.", my_rank, coord[0], coord[1]);
		MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, masterSize-1, MSG_RESPOND_ALIVE, world_comm);
		memset(buf, 0, 256 * sizeof(char));
		
		// SImulate a fault and terminate the loop 
		//if(counter == (my_rank+1) * 200){
		//	printf("[Process %d] Fault occurred\n", my_rank);
		//	fflush(stdout);
		//	break;
		//}
		
		//counter++;
		sleep(0);
	}
	
	printf("[Process %d] ended\n", my_rank);

    	MPI_Comm_free( &comm2D );
	return 0;
}
