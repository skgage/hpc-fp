#include "utils.h"

int main(int argc, char *argv[]) {

  //total size
  int n = 2048;

  //are errors turned on?
  bool errorsOn = false;
  //is checkpointing turned on?
  //TODO
  bool checkpointing = false;

  //Is redundancy turned on?
  //TODO
  bool redundancy = false;

  //checkpoint interval in iterations
  int checkInterval = 3;

  //Run until this many intervals is reached
  int iterationGoal = 100;

  int maxIterations = 1000;

  //time constants in milliseconds
  int writeCheckT = 1;
  int restartT = 1;

  //error mttf milliseconds
  double errorRate = .14;

  //open file for data
  FILE * output;
  output = fopen("output.txt", "w");
  //write header
  fprintf(output, "errors?, checkpointing?, redundancy?, n, numtasks, writeCheckT, restartT, checkInterval, intervalGoal, globalInterval, totalTime\n");

  //initialize mpi
  MPI_Init(&argc, &argv);

  //run a full cycle of iterations given constants
  fullCycle(n, errorsOn, checkpointing, redundancy, checkInterval, iterationGoal, maxIterations, writeCheckT, restartT, errorRate, output);

  errorsOn = true;
  checkpointing = true;

  fullCycle(n, errorsOn, checkpointing, redundancy, checkInterval, iterationGoal, maxIterations, writeCheckT, restartT, errorRate, output);

  redundancy = true;
  fullCycle(n, errorsOn, checkpointing, redundancy, checkInterval, iterationGoal, maxIterations, writeCheckT, restartT, errorRate, output);

  errorsOn = false;
  redundancy = true;
  fullCycle(n, errorsOn, checkpointing, redundancy, checkInterval, iterationGoal, maxIterations, writeCheckT, restartT, errorRate, output);


  //close MPI
  MPI_Finalize();

  //close output file
  fclose(output);

  return 0;
}
