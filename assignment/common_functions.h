#ifndef COMMONFUNCTIONS_H

#define COMMONFUNCTIONS_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>

// Used in MPI_CART_SHIFT to find neighbour process:
#define SHIFT_ROW 0
#define SHIFT_COL 1
// Displacement by 1:
#define DISP 1 
// Sensor Threshold:
#define SENSOR_THRESH 5
// Temperature Threshold:
#define TEMP_THRESH 80
// Random Temperature Generated Range:
#define MAX_TEMP_RANGE 100
#define MIN_TEMP_RANGE 30
// Message sent when a node is alive
#define MSG_RESPOND_ALIVE 2
// Values needed for fault detection
#define SLEEP_MICRO_SEC 0000000
#define wait_limit_sec	2
#define TERMINATION_FAULT 10 
#define TIME_THRESHOLD 50.0

#define SATELLITE_SIZE 100

//Struct of Values sent to Base Station
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
//int ndims = 2;
int dims[2];
  


// Initialization of functions used
//void sleep(int rank);
int base_station(MPI_Comm world_comm, MPI_Comm comm);
int slave_node(MPI_Comm world_comm, MPI_Comm comm);
void *ThreadFunc(void *pArg);
int compare(int rank, int temp, time_t timestamp);
int getCoordi(int rank, int columnSize);
int getCoordj(int rank, int columnSize);
void logRecord(int iter, int nodeRank, int satRank, time_t alertTime, int alertTemp, int alertType, int adjRanks[4], int adjTemps[4], time_t satTime, int satTemp, int numOfNodes, int columnSize);

void* FaultDetectProcess(void *pArg);

#endif
