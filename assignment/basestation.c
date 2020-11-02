#include "common_functions.h"

// Mutex for accessing global shared array
pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

// Boolean to check if POSIX thread should be terminated or not
int pthread_state = 0;

// Used to keep count of the number of values entered into the gloval shared array
int satelliteValueCount = 0;

// A structure that is used to record a satellite recording value
struct satValue{
    int sat_rank;
    int temp;
    time_t timestamp;
    };

// A fixed array of satellite value recordings
struct satValue satelliteValues[100];

// used for fault detection
pthread_mutex_t f_Mutex = PTHREAD_MUTEX_INITIALIZER;
int g_nslaves = 0;	
int activity_check = 1;

int base_station(MPI_Comm world_comm, MPI_Comm comm){
    int size;
    int slaveSize;
    
    int trueAlerts = 0;
    int falseAlerts = 0;
    
    // Get world size  
    MPI_Comm_size(world_comm, &size);
    MPI_Comm_size(world_comm, &slaveSize);
    size = size - 1;
    slaveSize = slaveSize-1;
    MPI_Status status;
    MPI_Status probe_status;
    
    // create the thread for fault detection part
    pthread_t fault_tid;
	pthread_mutex_init(&f_Mutex, NULL);
	pthread_create(&fault_tid, 0, FaultDetectProcess, NULL); // Create the thread for fault detection
    
    // flag used for MPI_IProbe to check if any messages are coming from WSN nodes
    int flag = 0;
        
    struct toSend recvMsg;
   
    // Create POSIX thread to simulate infrared imaging satellite
	pthread_t tid;
    pthread_create(&tid, 0, ThreadFunc, &size);
    
    // wait for messages from WSN nodes
   
   // fixed loop iterates 100 times
    for (int i = 0; i<100; i++){
    
    	// break if there is a fault detected
    	pthread_mutex_lock(&f_Mutex);
			if(activity_check == 0){
				break;
			}
		pthread_mutex_unlock(&f_Mutex);
    
        while(!flag){
        	
        	// break if there is a fault detected
        	pthread_mutex_lock(&f_Mutex);
				if(activity_check == 0){
					break;
				}
			    pthread_mutex_unlock(&f_Mutex);
        	
        	// Check for messages coming from WSN nodes
            MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
           
            // A message has been detected from WSN nodes
            if (flag) {
                // loopEnd determines the number of elements currently in the global shared array
                int loopEnd;
                int alertType = 0;
                
                // matchedValue holds the best match of the incoming WSN node information to the shared array
                struct satValue matchedValue;
                matchedValue.sat_rank = -1;
                
                // getting the number of elements currently in the array
                if(satelliteValueCount>=100){
                    loopEnd = 100;
                }
                else{
                    loopEnd = satelliteValueCount;
                }
                // receiving the structure containing WSN node information from the WSN node
                MPI_Recv(&recvMsg, sizeof(struct toSend), MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
                
               
                // lock global shared array
                pthread_mutex_lock(&g_Mutex);
                
                // loop through the number of elements currently in global shared array
                for (int j = 0; j<loopEnd; j++){
                    // if the WSN node rank and the satellite record rank matches
                    if (satelliteValues[j].sat_rank == recvMsg.node_rank) {
                        // set the new match to this record
                        matchedValue.sat_rank = satelliteValues[j].sat_rank;
                        matchedValue.temp = satelliteValues[j].temp;
                        matchedValue.timestamp = satelliteValues[j].timestamp;
                        
                        // if time difference is less than 50 seconds, this is already a match, so break
                        if (difftime(recvMsg.timestamp, satelliteValues[j].timestamp) < TIME_THRESHOLD) {
                            break;
                        }                  
                    }
                    
                  
                }
                // if a match was found in the satellite records that match the node rank
                if(matchedValue.sat_rank != -1){ 
                    // if temperature falls within temperature threshold
                    if((abs(matchedValue.temp-recvMsg.temp)) <= SENSOR_THRESH){
                        alertType = 1;
                        trueAlerts++;
                    }
                    else{
                        alertType = 0;
                        falseAlerts++;
                    }
                  
                }
                // log the record into the log file
                logRecord(i, recvMsg.node_rank, matchedValue.sat_rank, recvMsg.timestamp, recvMsg.temp, alertType, recvMsg.adjacentRanks, recvMsg.adjacentTemp, matchedValue.timestamp, matchedValue.temp, recvMsg.numOfNodes, recvMsg.colSize);
                
               // unlock global shared array
               pthread_mutex_unlock(&g_Mutex);
            }
        }
        // get another message from WSN node, so set flag to 0 again
        flag = 0;
    }
    printf("Number of true alerts: %d\n", trueAlerts);
    printf("Number of false alerts: %d\n\n", falseAlerts);
    // send a message to each proccessor, so that they terminate
    for(int i = 0; i <= size; i++){
            MPI_Send(&i, 1, MPI_INT, i, 0, world_comm);
    }
    
    // terminate POSIX thread
    pthread_state = 1;
    
    printf("MPI Master Process finished\n");
	fflush(stdout);
    pthread_join(fault_tid, NULL);
	pthread_mutex_destroy(&f_Mutex);
    return 0;
}


void logRecord(int iter, int nodeRank, int satRank, time_t alertTime, int alertTemp, int alertType, int adjRanks[4], int adjTemps[4], time_t satTime, int satTemp, int numOfNodes, int columnSize){
    
    // Append to log.txt file
	FILE *logFile = fopen("log.txt", "a+");
	double commTime;
	time_t currentTime;
	
	currentTime = time(NULL);
	// get difference between alert time and satellite recording time
	commTime = difftime(alertTime, satTime);
	
	// write to all information to log file
	fprintf(logFile, "------------------------------------------------------\n");
    fprintf(logFile, "Iteration : %d\n", iter);
    fprintf(logFile, "Logged Time : %s\n", ctime(&currentTime));
    fprintf(logFile, "Alert Reported Time : %s\n", ctime(&alertTime));
    if(alertType){
        fprintf(logFile, "Alert Type : %s\n", "True" );
    }
    else{
        fprintf(logFile, "Alert Type : %s\n", "False" );
    }
    
    fprintf(logFile, "%s\t\t %s\t\t %s\t\t\n", "Reporting Node", "Coord", "Temp");
    
    fprintf(logFile,"%d\t\t\t\t\t (%d, %d)\t\t %d\t\t\t\n\n", nodeRank, getCoordi(nodeRank,columnSize), getCoordj(nodeRank,columnSize) , alertTemp);
          
    fprintf(logFile, "%s\t\t %s\t\t %s\t\t\n", "Adjacent Node", "Coord", "Temp");
    
    for(int i = 0; i < 4; i++){
        // if there is a adjacent node
        if(adjRanks[i] != -2){
            fprintf(logFile,"%d\t\t\t\t\t (%d, %d)\t\t %d\t\t\t\n", adjRanks[i], getCoordi(adjRanks[i],columnSize), getCoordj(adjRanks[i],columnSize) , adjTemps[i]);
        }
    }
    fprintf(logFile, "\n");  
    
    // if there is a record of a satellite recording
    if(satRank != -1){
        fprintf(logFile, "Infrared Satellite Record Status : %s\n\n", "FOUND");
        fprintf(logFile, "Infrared Satellite Reporting Time : %s", ctime(&satTime));
        fprintf(logFile, "Infrared Satellite Reporting (Celsius): %d\n", satTemp);
        fprintf(logFile, "Infrared Satellite Coord : (%d, %d)\n\n", getCoordi(satRank,columnSize), getCoordj(satRank,columnSize));
        fprintf(logFile, "Communication Time (seconds) : %.2f\n", commTime);
    }
    else{
        fprintf(logFile, "Infrared Satellite Record Status : %s\n\n", "NOT FOUND FOR GIVEN COORDINATES");
    }
    
    fprintf(logFile, "Total Messages send between reporting node and base station : %d\n", 1);
    fprintf(logFile, "Number of adjacent matches to reporting node : %d\n", numOfNodes);
    fprintf(logFile, "------------------------------------------------------\n");
	fclose(logFile);
}

int getCoordi(int rank, int columnSize){
    int icoord = (int) rank/columnSize;
    
    return icoord;
}

int getCoordj(int rank, int columnSize){
    int jcoord = rank%columnSize;
    return jcoord;
}

// POSIX thread function that simulates infrared imaging satellite
void *ThreadFunc(void *pArg){
    //nnodes is the number of WSN nodes
    int nnodes;
    int iter = 0;
    int* p = (int*)pArg;
	nnodes = *p;
	
	while(1){
	    // if base station has completed its processing, exit the thread
	    if(pthread_state){
	        pthread_exit(NULL);
	    }
	    // lock global shared array
	    pthread_mutex_lock(&g_Mutex);
	    
	    // set seed value
        unsigned int seed = time(NULL) * (satelliteValueCount);
	    
	    // generating random temperature,rank(coordinates) and also the time and store in global shared array
	    satelliteValues[iter].temp = rand_r(&seed) % (MAX_TEMP_RANGE + 1 - MIN_TEMP_RANGE) + MIN_TEMP_RANGE;	          
	    satelliteValues[iter].sat_rank = rand_r(&seed) % (nnodes + 1);
	    time(&satelliteValues[iter].timestamp);
 
	    satelliteValueCount++;
	    iter++;
	    
	    // if globalSharedArray has been filled, start replacing records from index 0 again
	    if(satelliteValueCount == 100) {
	        iter = 0;
        }
        // unlock global shared array
	    pthread_mutex_unlock(&g_Mutex);
	    // sleep for 1 second
	    sleep(1);
	}
    return 0;
}

void* FaultDetectProcess(void *pArg)
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
	printf("Fault Prevention Initialized.\n");
	
	clock_gettime(CLOCK_MONOTONIC, &clock_time); 

	pthread_mutex_lock(&f_Mutex);
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
	pthread_mutex_unlock(&f_Mutex);
	
	int localSlavesCount = 0;
	while (activity_check == 1) {
		pthread_mutex_lock(&f_Mutex);
		if(g_nslaves <= 0){
			pthread_mutex_unlock(&f_Mutex);
			break;
		}
		pthread_mutex_unlock(&f_Mutex);
		
		MPI_Iprobe(MPI_ANY_SOURCE, MSG_RESPOND_ALIVE, MPI_COMM_WORLD, &flag, &probe_status);
		if(flag){
			MPI_Recv(buf, 256, MPI_CHAR, probe_status.MPI_SOURCE, MSG_RESPOND_ALIVE, MPI_COMM_WORLD, &status);
			
			clock_gettime(CLOCK_MONOTONIC, &clock_time); // get the current timestamp
			pSlaves_last_alive_timestamp_array[probe_status.MPI_SOURCE].tv_sec =  clock_time.tv_sec;
			pSlaves_last_alive_timestamp_array[probe_status.MPI_SOURCE].tv_nsec =  clock_time.tv_nsec;
			memset(buf, 0, 256 * sizeof(char));
		}
		pthread_mutex_lock(&f_Mutex);
			localSlavesCount = g_nslaves;
		pthread_mutex_unlock(&f_Mutex);
		
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
					pthread_mutex_lock(&f_Mutex);
					activity_check = 0;
					pthread_mutex_unlock(&f_Mutex);
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

