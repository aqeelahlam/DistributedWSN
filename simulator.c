#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include <time.h>

// Used in MPI_CART_SHIFT to find neighbour process:
#define SHIFT_ROW 0
#define SHIFT_COL 1
// Displacement by 1:
#define DISP 1 
// Maximum Iterations:
#define MAX_ITER 100
// Sensor Threshold:
#define SENSOR_THRESH 5
// Temperature Threshold:
#define TEMP_THRESH 80
// Random Temperature Generated Range:
#define MAX_TEMP_RANGE 100
#define MIN_TEMP_RANGE 65

#define SATELLITE_SIZE 100

// Initialization of functions used
void sleep(int rank);
int base_station(MPI_Comm world_comm, MPI_Comm comm);
int slave_node(MPI_Comm world_comm, MPI_Comm comm);
void *ThreadFunc(void *pArg);
int compare(int rank, int temp, time_t timestamp);
int getCoordi(int rank, int columnSize);
int getCoordj(int rank, int columnSize);
void logRecord(int iter, int nodeRank, int satRank, time_t alertTime, int alertTemp, int alertType, int adjRanks[4], int adjTemps[4], time_t satTime, int satTemp, int numOfNodes, int columnSize);

pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

int pthread_state = 0;

int satelliteValueCount = 0;

struct satValue{
    int sat_rank;
    int temp;
    time_t timestamp;
    };

/*
Struct of Values sent to Base Station
*/
struct toSend{
    int node_rank;
    int temp;
    int adjacentRanks[4];
    int adjacentTemp[4];
    int numOfNodes;
    time_t timestamp;
    int colSize;
    };
    
int nrows, ncols;
int ndims = 2;
int dims[2];
  
struct satValue satelliteValues[100];

int main(int argc, char *argv[]) {

    int size, rank;
    // The MPI Communicator Inititalized
    MPI_Comm new_comm;

    // Start up initial MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    
    

    

    if (argc == 3) {
        // SPECIFY NO OF ROWS AND COLUMNS through command line arguments
        nrows = atoi (argv[1]);
        ncols = atoi (argv[2]);
        
        dims[0] = nrows; //  number of rows 
        dims[1] = ncols; // number of columns
        // if col number*row number does not equal number of processes
        if( ((nrows*ncols)+1) != size) {
            // NUMBER OF PROCESSES NEED TO BE EQUAL NO OF ROWS * COLUMNS
            // RUN USING CLUSTER (mpirun -hostfile cluster w1q2 4 5)
            // if process is first process, print error message
            if(rank ==0) {
                printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows*ncols, size);
            }
            MPI_Finalize();
            return 0;
        }
        
    } else {
    // if command line arguments not given, set rows and column number
        nrows=ncols=(int)sqrt(size);
        dims[0]=dims[1]=0;
    }
    
    

    /*
    Here we Split the World Communicator into two:
    'rank == size -1' will return 0 or 1 (True or False)
    Color will either be:
    0: For Slaves 
    1: For Master
    */ 
    MPI_Comm_split(MPI_COMM_WORLD, rank == size-1, 0, &new_comm); 

    // The last rank is the Master
    if(rank == size-1)
        base_station(MPI_COMM_WORLD, new_comm);
    else
        slave_node(MPI_COMM_WORLD, new_comm);
    MPI_Finalize();
    return 0;
}


int slave_node(MPI_Comm world_comm, MPI_Comm comm){
    
    // Sizes of WORLD_COMM and Virtual Slave Topologies
    int worldSize, size;
    // Current Rank of the Slave
    int rank;

    int reorder, my_cart_rank, ierr;

    // This will hold the created 
    MPI_Comm comm2D;
    MPI_Status status;

    // Get the size of the Master Communicator
    MPI_Comm_size(world_comm, &worldSize);
    // Get the size of the Slave Communicator
    MPI_Comm_size(comm, &size);
    // Rank of Slave Communicator
    MPI_Comm_rank(comm, &rank);

    // 2 Dimension Grid for Virtual Topology
    int ndims = 2;

    // Array of Adjacent Node
    int adjacent[4] = {-1,-1,-1,-1};

    /* 
    Neighbouring Nodes:
    top - Top Neighbour
    bottom - Bottom Neighbour
    left - Left Neighbour
    right - Right Neighbour
    These variables hold the 'Rank' Values of the neighbouring nodes
    */
    int top, bottom;
    int left, right;

    int end_flag = 0;

    // Arrays to hold dimension, coordiantes and wrap around
    //int dims[ndims]; 
    int coord[ndims];
    int wrap_around[ndims];

    // Holds the rank of the Basestation
    int baseStationRank = worldSize-1;
    
    // HERE WE HAVE TO CHANGE TO CHANGE GRID SIZE
    //dims[0]=dims[1]=0;
    
    MPI_Dims_create(size, ndims, dims);

    if(rank == 0)
        printf("Root Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",rank ,size,dims[0],dims[1]);

    /* 
    Periodic shift is false- Meaning the top, bottom and left,right nodes cant 
    communicate with each other in a circular manner.
    */
    wrap_around[0] = wrap_around[1] = 0;

    /* This would re-order the ranking if set to one */
    reorder = 1;
    // Error boolean variable
    ierr = 0;
    /* 
    Make new communicator of which topology info is stored.
    Now onward we have a virtual topology communicator: comm2D
    */
    ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
    
    // if error is found creating cartesian topology, print error message
    if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);

    /* 
    This function will return the coordinate value (eg: (0,1)) if I pass the
    value for rank as 1. 
    Finds my coordinates in the cartesian communicator group 
    */
    MPI_Cart_coords(comm2D, rank, ndims, coord);

    // Use my cartesian coordinates to find my rank in cartesian group
    MPI_Cart_rank(comm2D, coord, &my_cart_rank);

    // Calculate Process ID for adjacent process:
    MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &top, &bottom);
    MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &left, &right);

    adjacent[0] = top;
    adjacent[1] = bottom;
    adjacent[2] = left;
    adjacent[3] = right;

    int adjacentNodes = 4;

    unsigned int seed = time(NULL)*rank;

    while(!end_flag){
        MPI_Iprobe(worldSize-1, 0, MPI_COMM_WORLD, &end_flag, &status);
        if(end_flag){
            break;
        } 

        struct toSend packet;
        int received_temperature[4] = {-1,-1,-1,-1};
        int numOfNodesAboveThreshold = 0;
        int randomTemp = 0;
        // Generate random temperature
        randomTemp = rand_r(&seed) % (MAX_TEMP_RANGE + 1 - MIN_TEMP_RANGE) + MIN_TEMP_RANGE; 

        // Perform sending operation without having adjacent nodes to receive
        for(int i = 0; i < adjacentNodes; i++){
            MPI_Send(&randomTemp, 1, MPI_INT, adjacent[i], 0, comm2D);
        }

        // If the random temperature 
        if(randomTemp > TEMP_THRESH){
            for(int i = 0; i < adjacentNodes; i++){
                MPI_Recv(&received_temperature[i], 1, MPI_INT, adjacent[i], 0, comm2D, &status);
            }

            // Check abnoramlities in temp : 
            for(int j = 0; j < adjacentNodes; j++){
                if((abs(randomTemp-received_temperature[j])) <= SENSOR_THRESH){
                    numOfNodesAboveThreshold++;
                }
            }
        }


        /*
        if at least two or more neighbourhood nodes match the sensor readings of the local node
        */
	    if(numOfNodesAboveThreshold >= 2){
            // Initialize structure members
            packet.node_rank = rank;
            packet.temp = randomTemp;
            packet.numOfNodes = numOfNodesAboveThreshold;
            time(&packet.timestamp);
            packet.colSize = ncols;
            for(int i = 0; i < adjacentNodes; i ++){
                packet.adjacentRanks[i] = adjacent[i];
                packet.adjacentTemp[i] = received_temperature[i];   
            }
            // Alert the base station
            //printf("Rank: %d, Temp: %d, Time %s", packet.node_rank, packet.temp, ctime(&packet.timestamp));
            MPI_Send(&packet, sizeof(struct toSend), MPI_CHAR, baseStationRank, 0, world_comm);           
        }
	    sleep(1);
    }
    return 0;
          
}

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

