/**
 * Test:
########## Matriz 3000
# Laptop
6 cores 71.52           1.2 min

# OpenShift
        245.9           4 min
6 cores 153.945103      2.56 min
__________________________________________-
########## Matriz 2000
# Laptop
6 Cores             17.97 seg
1 core              63.59
sin threads         60.44

# OpenShift
6 cores             36.51 seg
6(3) cores          53.204 seg
8(6) cores          40 seg
1 core              2.77 min
Sin threads         167.259 seg -> 2.78 min
*/
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#define N 2000

double A[N][N];
double B[N][N];
double C[N][N];

void print_m(double M[N][N]) {
  fprintf(stderr, "\n");
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      fprintf(stderr, "%lf ", M[i][j]);
    }
    fprintf(stderr, "\n");
  }
}

int main() {
  struct timeval t_ini, t_end;

  // init
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      A[i][j] = i + j;
      B[i][j] = i + j;
      C[i][j] = 0.0;
    }
  }

  // print_m(A);
  // print_m(B);

  fprintf(stderr, "%dx%d\n", N, N);
  omp_set_num_threads(6);

  // mult
  gettimeofday(&t_ini, NULL);
#pragma omp parallel for
  for (int i = 0; i < N; i++) {
    if (i == 0) {
      int nt = omp_get_num_threads();
      fprintf(stderr, "num threads: %d\n", nt);
    }

    for (int j = 0; j < N; j++) {
      for (int k = 0; k < N; k++) {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  gettimeofday(&t_end, NULL);
  fprintf(stderr, "Time: %f s\n",
          (t_end.tv_sec - t_ini.tv_sec) +
              (t_end.tv_usec - t_ini.tv_usec) / 1000000.0);
  // print_m(C);

  while (true) {
    sleep(60);
  }
}