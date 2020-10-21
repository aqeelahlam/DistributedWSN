#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

#define SHIFT_ROW 0
#define SHIFT_COL 1

#define DISP 1

void *ThreadFunc(void *pArg);
void b_station();
void WSN_node();

int main(int argc, char *argv[]) {

    int size, rank, reorder, my_cart_rank, ierr;
    int *prime_array;
    int ndims=2;
    // number of rows and number of columns
    int nrows, ncols;
    int nbr_i_lo, nbr_i_hi;
    int nbr_j_lo, nbr_j_hi;
    MPI_Comm comm2D;
    int dims[ndims],coord[ndims];
    int wrap_around[ndims];
    
    
    /* start up initial MPI environment */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    
    
    if (argc == 3) {
        // SPECIFY NO OF ROWS AND COLUMNS through command line arguments
        nrows = atoi (argv[1]);
        ncols = atoi (argv[2]);
        
        dims[0] = nrows; /* number of rows */
        dims[1] = ncols; /* number of columns */
        // if col number*row number does not equal number of processes
        if( (nrows*ncols) != size) {
            //NUMBER OF PROCESSES NEED TO BE EQUAL NO OF ROWS * COLUMNS
            // RUN USING CLUSTER (mpirun -hostfile cluster w1q2 4 5)
            // if process is first process, print error message
            if(rank ==0) {
                printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows*ncols,size);
            }
            MPI_Finalize();
            return 0;
        }
        
    } else {
    // if command line arguments not given, set rows and column number
        nrows=ncols=(int)sqrt(size);
        dims[0]=dims[1]=0;
    }
    

    /************************************************************
    */
    /* create cartesian topology for processes */
    /************************************************************
    */
    MPI_Dims_create(size, ndims, dims);
    if(rank==0)
        printf("Root Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n",rank ,size,dims[0],dims[1]);
        
    /* create cartesian mapping */
    wrap_around[0] = wrap_around[1] = 0; /* periodic shift is
    .false. */
    reorder = 1;
    // error boolean variable
    ierr =0;
    // make new communicator of which topology info is stored
    ierr = MPI_Cart_create(MPI_COMM_WORLD, ndims, dims,
    wrap_around, reorder, &comm2D);
    // if error is found creating cartesian topology, print error message
    if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);
    /* find my coordinates in the cartesian communicator group */

    MPI_Cart_coords(comm2D, rank, ndims, coord);
    /* use my cartesian coordinates to find my rank in cartesian
    group*/
    MPI_Cart_rank(comm2D, coord, &my_cart_rank);
    /* get my neighbors; axis is coordinate dimension of shift */
    /* axis=0 ==> shift along the rows: P[my_row-1]: P[me] :
    P[my_row+1] */
    /* axis=1 ==> shift along the columns P[my_col-1]: P[me] :
    P[my_col+1] */

    // CALCULATE PROCESS ID FOR NEIGHBOUR process
    MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi
    );
    MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi
    );
    
    /*
    if(rank == size-1) {
        b_station();
        }
    else {
        WSN_node();
        }
    */
    
    MPI_Finalize();
    return 0;
}

