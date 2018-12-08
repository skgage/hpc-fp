#include <chrono>
#include <thread>
#include "mpi.h"

#include "utils.h"

int main(int argc, char *argv[]) {

  int numtasks, rank;

  //total size
  int n = 1024;

  //is checkpointing turned on?
  //TODO
  bool checkpointing;

  //Is redundancy turned on?
  //TODO
  bool redundancy;

  //checkpoint interval in iterations
  int checkInterval = 10;

  //Run until this many intervals is reached
  int iterationGoal = 1000;

  int maxIterations = 1000000;

  //time constants in milliseconds
  int writeCheckT = 1;
  int restartT = 1;

  //error mttf milliseconds
  double errorRate = .1;



  MPI_Init(&argc, &argv);

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //printf("running thread %d\n", rank);

  MPI_Status st;

  int MPIErrorStatus;

  //printf("numtasks: %d\n", numtasks);

  //local number of rows size
  int m = n/numtasks;

  //To track total intervals including those that get redone
  int globalIteration = 0;

  //To track progress
  int currentIteration = 0;

  //for use in error calcs
  double t1 = 0;
  double t2 = 0;

  //Fill A, x, and b in Ax = b
  std::vector<int> a;
  fillVector(a, n*m);

  std::vector<float> xOld;
  fillVector(xOld, n);

  std::vector<float> xNew;
  fillVector(xNew, m);

  std::vector<int> b;
  fillVector(b, n);

  //make vector to track error messages
  std::vector<int> errors;
  errors.resize(numtasks, 0);

  //true means there is an error
  int localError = 0;

  //for use in global time tracking
  double t1Global = MPI_Wtime();
  double t2Global = 0;

  while(currentIteration < iterationGoal){
    //dummy calculation
    jacobi_iteration(n, m, a, xOld, xNew, b);

    //update t2
    t2 = MPI_Wtime();

    //check for local error and update t1
    localError = isError(errorRate, t2-t1);
    t1 = MPI_Wtime();

    //send error value
    MPIErrorStatus = MPI_Allgather(&localError, 1, MPI_INT, errors.data(), 1, MPI_INT, MPI_COMM_WORLD);

    //check for error
    if (checkErrorVector(errors, numtasks)) {
      //reset interval to previous check point
      currentIteration -= currentIteration % checkInterval;

      //spend time for restart
      std::this_thread::sleep_for(std::chrono::milliseconds(restartT));
    }
    else{
      //TODO send values

      //increment interval
      currentIteration += 1;

      //check if checkpoint interval
      if (currentIteration % checkInterval == 0) {
        //spend time for checkpointing
        std::this_thread::sleep_for(std::chrono::milliseconds(writeCheckT));
      }
    }
    // Update globalInterval counter
    globalIteration += 1;
    // if (rank == 0) {
    //   printf("totalIteration: %d, localIteration: %d\n", globalIteration, currentIteration);
    // }

    //check max maxIterations
    if (globalIteration > maxIterations) {
      break;
    }
  }

  //Once while loop has completed the first thread can report its values
  if (rank == 0) {
    t2Global = MPI_Wtime();
    double totalTime = t2Global - t1Global;

    //n, numtasks, writeCheckT, restartT, checkInterval, intervalGoal, globalInterval, totalTime;
    printf("%d, %d, %d, %d, %d, %d, %d, %f\n", n, numtasks, writeCheckT, restartT, checkInterval, iterationGoal, globalIteration, totalTime);
  }

  //close mpi
  MPIErrorStatus = MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}
