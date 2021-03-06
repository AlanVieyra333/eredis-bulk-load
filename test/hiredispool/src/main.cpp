#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

#include <adapters/libev.h>

#include "config.h"

const char *redis_host;
int redis_port;
const char *redis_pass;
int redis_database;

int get_lines(const char *filename){
  int lines = 0;
  long phone;
  char value[MAXCHAR];

  FILE *file = fopen(filename, "r");
  if (file == NULL) return -1;

  while (fscanf(file, "%ld|%[^\n]^\n", &phone, value) != EOF) lines++;

  fclose(file);

  return lines;
}

void read_file() {
  int lines;
  int phone_count = 0;

  log_(L_INFO | L_CONS, "Cargando registros de %s...\n", filename);

  lines = get_lines(filename);

  //log_(L_DEBUG, "Lineas: %d\n", lines);

  // Thread management.
#pragma omp parallel shared(phone_count)
  {
    redisAsyncContext *ac = redis_init(redis_host, redis_port, redis_pass, redis_database);
    int nt = omp_get_num_threads();
    int iam = omp_get_thread_num();
    int th_block = lines / nt;
    int line_ini = th_block * iam;
    int line_end = iam != nt - 1 ? th_block * (iam + 1) : lines;

    long phone_ini, phone_end;
    char key[11];
    char value[MAXCHAR];
    char line[MAXCHAR];

    if ( iam == 0 ) log_(L_INFO | L_CONS, "Hilos usados: %d\n", nt);

    FILE *file = fopen(filename, "r");

    for (int curr_line = 0; curr_line < line_ini ; curr_line++) fgets(line, MAXCHAR, file);

    //log_(L_DEBUG | L_CONS, "%d) Lines: %d - %d\n", iam, line_ini, line_end);

    int i = 0;
    // Expansion de registros en el intervalo.
    for (i = line_ini; i < line_end; i++) {
      fscanf(file, "%ld|%ld|%[^\n]s", &phone_ini, &phone_end, value);

      if (phone_end - phone_ini < 10000) {
#pragma omp critical
        phone_count += phone_end - phone_ini + 1;

        //log_(L_DEBUG | L_CONS, "%d) phones: %ld - %ld\n", iam, phone_ini, phone_end);

        for (long phone = phone_ini; phone <= phone_end; phone++) {
          sprintf(key, "%ld", phone);

          /* Cargar a Redis */
          redisAsyncCommand(ac, NULL, NULL, "SET %s %s", key, value);
        }
      }
    }

    fclose(file);

    /* Disconnect redis sock */
    redisAsyncDisconnect(ac);
  }

  log_(L_INFO | L_CONS, "Carga completa. Total de registros: %ld\n",
       phone_count);
}

int main(int argc, char **argv)
{
  fprintf(stderr, "#############################\n");
  fprintf(stderr, "# Data load - Series v%s #\n", VERSION);
  fprintf(stderr, "#############################\n\n");

  if (argc < 5) {
    log_(L_WARN,
         "Use: ./main.o <REDIS_HOST> <REDIS_PORT> <REDIS_PASS> <REDIS_DATABASE> <FILE_NAME>\n");
    exit(1);
  }

  redis_host = (argc > 1) ? argv[1] : "127.0.0.1";
  redis_port = (argc > 2) ? atoi(argv[2]) : 6379;
  redis_pass = (argc > 3) ? argv[3] : "";
  redis_database = (argc > 4) ? atoi(argv[4]) : 0;
  filename = (argc > 5) ? argv[5] : "";

  log_init();
  signal_init();
  log_(L_INFO | L_CONS, "Inicializado correctamente.\n");

  file_watcher_init();
  //ev_loop(EV_DEFAULT_ 0);

  return 0;
}
