#include "common_functions.h"

pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

int pthread_state = 0;

int satelliteValueCount = 0;

struct satValue{
    int sat_rank;
    int temp;
    time_t timestamp;
    };

struct satValue satelliteValues[100];

int base_station(MPI_Comm world_comm, MPI_Comm comm){
    int size;
    //struct satValue s1 = {0, 0, 0};
    //*satelliteValues = malloc(100 * sizeof(s1));
    //*satelliteValues = (struct satValue*)malloc(100 * sizeof(struct satValue));
    
    MPI_Comm_size(world_comm, &size);
    
    size = size - 1;
            
    MPI_Status status;
    
    int flag = 0;
    //int recvMsg = 0;
    
    struct toSend recvMsg;
   
	pthread_t tid;
    pthread_create(&tid, 0, ThreadFunc, &size);
    
    // wait for messages from WSN nodes
   
   // fixed loop iterates 100 times
    for (int i = 0; i<100; i++){
        
        while(!flag){
            MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
           
            if (flag) {
                int loopEnd;
                int alertType = 0;
                
                struct satValue matchedValue;
                matchedValue.sat_rank = -1;
                
                
                if(satelliteValueCount>=100){
                    loopEnd = 100;
                }
                else{
                    loopEnd = satelliteValueCount;
                }
                MPI_Recv(&recvMsg, sizeof(struct toSend), MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
                //printf("Temperature: %d, Rank: %d, Time: %ld, top: %d \n", recvMsg.temp, recvMsg.node_rank, recvMsg.timestamp, recvMsg.adjacentRanks[0]);
                //compare(recvMsg.node_rank, recvMsg.temp, recvMsg.timestamp);
               
                
                pthread_mutex_lock(&g_Mutex);
                //printf("Loop : %d", loopEnd);
                for (int j = 0; j<loopEnd; j++){
                    //printf("Satellite Rank: %d, Satellite Temp: %d\n", satelliteValues[i].sat_rank, satelliteValues[i].temp);
                    if (satelliteValues[j].sat_rank == recvMsg.node_rank) {
                        matchedValue.sat_rank = satelliteValues[j].sat_rank;
                        matchedValue.temp = satelliteValues[j].temp;
                        matchedValue.timestamp = satelliteValues[j].timestamp;
                        
                        if (difftime(recvMsg.timestamp, satelliteValues[j].timestamp) < 50.0) {
                            
                            break;
                        }                  
                    }
                    
                  
                }
                if(matchedValue.sat_rank != -1){
                    /*
                    printf("Satellite Rank: %d, Satellite Temp: %d\n", matchedValue.sat_rank, matchedValue.temp);
                    printf("Satellite time: %s, Node time: %s\n", ctime(&matchedValue.timestamp), ctime(&recvMsg.timestamp));
                    printf("Diff time: %f ", (difftime(recvMsg.timestamp, matchedValue.timestamp)));
                    */
                    if((abs(matchedValue.temp-recvMsg.temp)) <= SENSOR_THRESH){
                        //printf("Temp diff: %d", (abs(matchedValue.temp-recvMsg.temp)));
                        alertType = 1;
                    }
                    else{
                        alertType = 0;
                    }
                  
                }
                logRecord(i, recvMsg.node_rank, matchedValue.sat_rank, recvMsg.timestamp, recvMsg.temp, alertType, recvMsg.adjacentRanks, recvMsg.adjacentTemp, matchedValue.timestamp, matchedValue.temp, recvMsg.numOfNodes, recvMsg.colSize);
                
                
                
                
                /*
                printf("Node Rank: %d, Node Temp: %d\n", recvMsg.node_rank, recvMsg.temp);
                
                
                printf("Alert type: %d\n\n", alertType);
                */
                
                /*
              
                pthread_mutex_unlock(&g_Mutex);
                printf("Rank: %d\n", recvMsg.node_rank);
                printf("WSN Alert Time: %s\n", ctime(&recvMsg.timestamp));
                printf("Satellite Capture Time: %s\n", ctime(&satelliteValues[i].timestamp));
                printf("Alert type: %d\n", alertType);
                printf("WSN Temp: %d\n", recvMsg.temp);
                printf("Satellite Temp: %d\n\n", satelliteValues[i].temp);
                */
               pthread_mutex_unlock(&g_Mutex);
            }
        }
        flag = 0;
    }

    for(int i = 0; i <= size; i++){
            MPI_Send(&i, 1, MPI_INT, i, 0, world_comm);
    }
  
    pthread_state = 1;
    
    //pthread_join(tid, NULL);
    return 0;
}


void logRecord(int iter, int nodeRank, int satRank, time_t alertTime, int alertTemp, int alertType, int adjRanks[4], int adjTemps[4], time_t satTime, int satTemp, int numOfNodes, int columnSize){
    
	FILE *logFile = fopen("log.txt", "a+");
	double commTime;
	time_t currentTime;
	
	currentTime = time(NULL);
	commTime = difftime(alertTime, satTime);
	
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
        if(adjRanks[i] != -2){
            fprintf(logFile,"%d\t\t\t\t\t (%d, %d)\t\t %d\t\t\t\n", adjRanks[i], getCoordi(adjRanks[i],columnSize), getCoordj(adjRanks[i],columnSize) , adjTemps[i]);
        }
    }
    fprintf(logFile, "\n");  
    
    
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


void *ThreadFunc(void *pArg){
    int nnodes;
    int iter = 0;
    int* p = (int*)pArg;
	nnodes = *p;
	
	while(1){
	    if(pthread_state){
	        pthread_exit(NULL);
	    }
	    pthread_mutex_lock(&g_Mutex);
	    
        unsigned int seed = time(NULL) * (satelliteValueCount);
	    //printf("Count: %d\n", satelliteValueCount); 
        /*scaling the output of rand_r() to be in between MIN_RANGE and MAX_RANGE and assigning it to a position in the list */
	    
	    satelliteValues[iter].temp = rand_r(&seed) % (MAX_TEMP_RANGE + 1 - MIN_TEMP_RANGE) + MIN_TEMP_RANGE;
		          
	    satelliteValues[iter].sat_rank = rand_r(&seed) % (nnodes + 1);
	    
	    time(&satelliteValues[iter].timestamp);
	    
        
        //printf("Temp: %d\n", satelliteValues[iter].temp);
	    //printf("Rank: %d\n", satelliteValues[iter].sat_rank);
	    //printf("Time: %s\n\n", ctime(&satelliteValues[iter].timestamp));
	    
	    satelliteValueCount++;
	    iter++;
	    
	    
	    if(satelliteValueCount == 100) {
	        iter = 0;
        }
	    pthread_mutex_unlock(&g_Mutex);
	    sleep(1);
	}
    return 0;
}

int compare(int rank, int temp, time_t timestamp) {
    //printf("Rank: %d", rank);
    pthread_mutex_lock(&g_Mutex);
    for (int i = 0; i<100; i++){
        if (satelliteValues[i].sat_rank == rank) {
            printf("Satellite time: %ld, Node time: %ld\n", satelliteValues[i].timestamp, timestamp);
            printf("Rank: %d, Time diff: %f\n\n", rank, difftime(timestamp, satelliteValues[i].timestamp));
            //printf("Rank1: %d, Rank2: %d\n", rank, satelliteValues[i].sat_rank);
        }
    } 
    pthread_mutex_unlock(&g_Mutex);
 
    return 0;
}
