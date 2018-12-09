#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <vector>       /* vector */
#include <math.h>
#include <random>
#include <chrono>
#include <thread>
#include "mpi.h"
#include "iostream"

void fillVector(std::vector<int> &v, int n);

void fillVector(std::vector<float> &x, int n);

int matIX(int n, int x, int y);

void jacobi_iteration(int n, int m, const std::vector<int> &a,
  std::vector<float> &xOld, std::vector<float> &xNew, const std::vector<int> &b);

int isError(double mttf, double time, std::default_random_engine generator);

bool checkErrorVector(const std::vector<int> &e, int p, bool rendundancys);

void fullCycle(int n, bool errorsOn, bool checkpointing, bool redundancy, int checkInterval, int iterationGoal, int maxIterations, int writeCheckT, int restartT, double errorRate, FILE * output);

#endif
