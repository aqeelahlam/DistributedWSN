#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

// Used in MPI_CART_SHIFT to find neighbour process:
#define SHIFT_ROW 0
#define SHIFT_COL 1
// Displacement by 1:
#define DISP 1 
// Maximum Iterations:
#define MAX_ITER 10
// Sensor Threshold:
#define SENSOR_THRESH 5
// Temperature Threshold:
#define TEMP_THRESH 80

void sleep(int rank);

//int base_station(MPI_Comm world_comm, MPI_Comm comm);
int slave_node(MPI_Comm world_comm, MPI_Comm comm);

int main(int argc, char *argv[]) {

    int size, rank;
    // The MPI Communicator Inititalized
    MPI_Comm new_comm;
    /* start up initial MPI environment */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    /*
    int nrows, ncols;
    
    int ndims = 2;
    
    int dims[ndims];
    

    if (argc == 3) {
        // SPECIFY NO OF ROWS AND COLUMNS through command line arguments
        nrows = atoi (argv[1]);
        ncols = atoi (argv[2]);
        
        dims[0] = nrows; //  number of rows 
        dims[1] = ncols; // number of columns
        // if col number*row number does not equal number of processes
        if( (nrows*ncols) != size) {
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
    
    */

    


    /*
    'rank == size -1' will return 0 or 1 (True or False)
    Color will either be:
    0: For Slaves 
    1: For Master
    */ 

    MPI_Comm_split(MPI_COMM_WORLD, rank == size-1, 0, &new_comm); 

    // The last rank is the Master
    
    /*
    if(my_rank == size-1)
        master_io(MPI_COMM_WORLD, new_comm);
    else
    */
        slave_node(MPI_COMM_WORLD, new_comm);
    MPI_Finalize();
    return 0;
}




int slave_node(MPI_Comm world_comm, MPI_Comm comm){

    // Sizes of WORLD_COMM and Virtual Slave Topologies
    int worldSize, size;

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
    // To generate Random Value Between 65 and 100
    int maximum_num = 100;
    int minimum_num = 65;
    
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

    // Arrays to hold dimension, coordiantes and wrap around
    int dims[ndims], coord[ndims];
    int wrap_around[ndims];
    int iteration = 1;
    int baseStationRank;
    
    dims[0]=dims[1]=0;
    
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
    /* 
    Use my cartesian coordinates to find my rank in cartesian group
    */
    MPI_Cart_rank(comm2D, coord, &my_cart_rank);
    
    /* Get my neighbors; axis is coordinate dimension of shift */
    
    /* 
    axis=0 ==> shift along the rows: P[my_row-1]: P[me] : P[my_row+1] 
    */
    
    /* 
    axis=1 ==> shift along the columns P[my_col-1]: P[me] : P[my_col+1] 
    */

    // CALCULATE PROCESS ID FOR NEIGHBOUR process
    MPI_Cart_shift(comm2D, SHIFT_ROW, DISP, &top, &bottom);
    MPI_Cart_shift(comm2D, SHIFT_COL, DISP, &left, &right);

    adjacent[0] = top;
    adjacent[1] = bottom;
    adjacent[2] = left;
    adjacent[3] = right;

    unsigned int seed = time(NULL);

    while(iteration <= MAX_ITER){

        int received_temperature[4] = {0,0,0,0};
        int numOfNodesAboveThreshold = 0;
        int randomTemp = 0;
        // Generate random temperature
        randomTemp = rand_r(&seed) % (maximum_num + 1 - minimum_num) + minimum_num; 

        // Perform sending operation without having adjacent nodes to receive
        for(int i = 0; i < 4; i++){
            MPI_Send(&randomTemp, 1, MPI_INT, adjacent[i], 0, comm2D);
            //printf("Temperature: %d ", adjacent[i]);
        }

        // If the random temperature 
        if(randomTemp > TEMP_THRESH){
            for(int i = 0; i < 4; i++){
                MPI_Recv(&received_temperature[i], 1, MPI_INT, adjacent[i], 0, comm2D, &status);
                
            }

            // Check abnoramlities in temp : 
            for(int j = 0; j < 4; j++){
                if((abs(randomTemp-received_temperature[j])) > SENSOR_THRESH){
                    numOfNodesAboveThreshold++;
                }
            }
            //printf("blah: %d ", numOfNodesAboveThreshold);
        }

        // Send the number of nodes to BaseStation
	    if(numOfNodesAboveThreshold >= 2){
            //MPI_Send(&numOfNodesAboveThreshold, 1, MPI_INT, worldSize-1, 0, world_comm);
            printf("RANK: %d\n", numOfNodesAboveThreshold);
        }
        
	    sleep(1);
    }
          
}
    
    
    
    
    
    
