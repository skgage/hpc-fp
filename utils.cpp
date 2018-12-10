#include "utils.h"

void fillVector(std::vector<int> &v, int n){
  srand(time(NULL));
  for (size_t i = 0; i < n; i++) {
    v.push_back(rand());
  }
  return;
}

void fillVector(std::vector<float> &x, int n){
  srand(time(NULL));
  for (size_t i = 0; i < n; i++) {
    x.push_back(0);
  }
  return;
}

int matIX(int n, int x, int y){
  return y + n * x;
}

void jacobi_iteration(int n, int m, const std::vector<int> &a,
  std::vector<float> &xOld, std::vector<float> &xNew, const std::vector<int> &b){
    //m = local size of x and b
  float sum = 0;

  for (size_t i = 0; i < m; i++) {

    sum = b.at(i);

    for (size_t j = 0; j < n; j++) {
      if (j != i){
        sum -= a.at(matIX(n, i, j))*xOld.at(j);
      }
    }

    xNew.at(i) = sum/a.at(matIX(n, i, i));
  }

  return;
}

int isError(double mttf, double time, std::mt19937 &gen){
  // Assuming f(t) = lambda * e^(-lambda * t)
  double lam = 1/mttf;
  double failProb = 1 - exp(-lam*time);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //printf("rank: %d fail prob: %f\n", rank, failProb);

  //Gives true of false based on probability
  //std::default_random_engine generator;
  std::bernoulli_distribution distribution(failProb);

  if (distribution(gen)) {
    //printf("rank: %d error\n", rank);
    //this means there was an error
    return 1;
  }

  else{
    return 0;
  }

}

bool checkErrorVector(const std::vector<int> &e, int p, bool redundancy){
  //printf("in Check Error Vector\n");
  bool check = false;

  //if rendundancy is on then both a node at i and it's pair at i+1 need to have an error
  if (redundancy) {
    // for (size_t i = 0; i < p; i += 2) {
    //   std::cout<<e.at(i)<<", " <<e.at(i+1)<<", ";
    // }
    // std::cout<<std::endl;
    for (size_t i = 0; i < p; i += 2) {
      if (e.at(i) == 1 && e.at(i+1) == 1) {
        check = true;
        break;
      }
    }
  }
  //Without redundancy any single node with and error cause a restart
  else{
    // for (size_t i = 0; i < p; i ++) {
    //   std::cout<<e.at(i)<<", ";
    // }
    // std::cout<<std::endl;
    for (size_t i = 0; i < p; i++) {
      if (e.at(i) == 1) {
        check = true;
        break;
      }
    }
  }
  return check;
}

void fullCycle(int n, bool errorsOn, bool checkpointing, bool redundancy, int checkInterval, int iterationGoal, int maxIterations, int writeCheckT, int restartT, double errorRate, FILE * output){
  int numtasks, rank;

  MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  //printf("running thread %d\n", rank);

  MPI_Status st;

  int MPIErrorStatus;

  //printf("numtasks: %d\n", numtasks);

  //local number of rows size
  int m;
  if (redundancy) {
    m = n/(numtasks/2);
  }
  else{
    m = n/numtasks;
  }

  //To track total intervals including those that get redone
  int globalIteration = 0;

  //To track progress
  int currentIteration = 0;

  std::random_device device;
  std::mt19937 gen(device()*(rank+1));
  // std::default_random_engine generator(time(0)*(rank+1));

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

  //for use in error calcs
  double t1 = MPI_Wtime();
  double t2 = 0;

  while(currentIteration < iterationGoal && globalIteration < maxIterations){
    //dummy calculation
    jacobi_iteration(n, m, a, xOld, xNew, b);

    //update t2
    t2 = MPI_Wtime();

    //check for local error and update t1
    if (errorsOn) {
      localError = isError(errorRate, t2-t1, gen);
    }
    t1 = MPI_Wtime();

    //send error value
    MPIErrorStatus = MPI_Allgather(&localError, 1, MPI_INT, errors.data(), 1, MPI_INT, MPI_COMM_WORLD);

    //check for error
    if (checkErrorVector(errors, numtasks, redundancy)) {
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
      if (currentIteration % checkInterval == 0 && checkpointing) {
        //spend time for checkpointing
        std::this_thread::sleep_for(std::chrono::milliseconds(writeCheckT));
      }
    }
    // Update globalInterval counter
    globalIteration += 1;
    // if (rank == 0) {
    //   printf("totalIteration: %d, localIteration: %d\n", globalIteration, currentIteration);
    // }

    MPIErrorStatus = MPI_Barrier(MPI_COMM_WORLD);

  }

  //Once while loop has completed the first thread can report its values
  if (rank == 0) {
    t2Global = MPI_Wtime();
    double totalTime = t2Global - t1Global;

    //errors?, checkpointing?, redundancy?, n, numtasks, writeCheckT, restartT, checkInterval, intervalGoal, globalInterval, totalTime;
    fprintf(output, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %f\n", errorsOn, checkpointing, redundancy, n, numtasks, writeCheckT, restartT, checkInterval, iterationGoal, globalIteration, totalTime);
    printf("%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %f\n", errorsOn, checkpointing, redundancy, n, numtasks, writeCheckT, restartT, checkInterval, iterationGoal, globalIteration, totalTime);
  }

  //close mpi
  MPIErrorStatus = MPI_Barrier(MPI_COMM_WORLD);

  return;
}
