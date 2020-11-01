#include "common_functions.h"


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





