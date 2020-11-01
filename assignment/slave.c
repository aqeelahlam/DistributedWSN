#include "common_functions.h"

int slave_node(MPI_Comm world_comm, MPI_Comm comm){
    
    // Sizes of WORLD_COMM and Virtual Slave Topologies
    int worldSize, size;
    // Current Rank of the Slave
    int rank;

    int reorder, my_cart_rank, ierr;
    char buf[256];

    // This will hold the created 
    MPI_Comm comm2D;
    MPI_Status status;

    memset(buf, 0, 256 * sizeof(char));
    
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
    
    // variables used for fault detection
    MPI_Status fault_status;
    int fault_flag;
    

    while(!end_flag){
    	MPI_Iprobe(baseStationRank, TERMINATION_FAULT, world_comm, &fault_flag, &fault_status);
    	if(fault_flag){
			printf("[Process %d] Fault detected\n", rank);
			break;
		}
        MPI_Iprobe(baseStationRank, 0, MPI_COMM_WORLD, &end_flag, &status);
        if(end_flag){
            break;
        } 
        
        // To notify base station that this is active
		sprintf( buf, "Slave %d at Coordinate: (%d, %d). ALIVE.", rank, coord[0], coord[1]);
		MPI_Send(buf, strlen(buf) + 1, MPI_CHAR, baseStationRank, MSG_RESPOND_ALIVE, world_comm);
		memset(buf, 0, 256 * sizeof(char));

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
    printf("[Process %d] ended\n", rank);

    MPI_Comm_free( &comm2D );
    return 0;
       
}
