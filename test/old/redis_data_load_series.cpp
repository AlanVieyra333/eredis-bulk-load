/**
 * Programa para leer registros de un archivo txt y agregarlos en un servidor
 * Redis.
 * Prueba:
 * Tiempo en cargar 200 millones: 25 min. prox.
 *
 * @version 1.7 Selecciona base de datos dentro de redis.
 * @date 22/05/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
 */

#include <eredis.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

#include "FileWatcher.h"
#include "log.h"

#define MAXCHAR 300
#define DATA_BLOCK 10000000  // Each DATA_BLOCK reg. reconnect to Redis server.
#define U_SLEEP 5            // Sleep 5us.
#define VERSION "1.7.1"
#define WORKDIR "/data"

char *filename, *redis_host, *redis_pass;
int redis_port, redis_database = 0;
int redis_set_count = 0;
int redis_cmd_fail = 0;

void sigint_handler(int sig_no) {
  log_(L_INFO | L_CONS, "CTRL-C pressed.\n");
  
  // if (e) {
  //   eredis_shutdown(e);
  // }

  signal(SIGINT, SIG_DFL);
  kill(getpid(), SIGINT);
}

void signal_conf() {
  /* to interrupt */
  signal(SIGINT, sigint_handler);
}

bool isConnected(eredis_t* &e) {
  bool isConnected = false;

  if (e) {
    if (eredis_pc_cmd(e, "SELECT %d", redis_database) == EREDIS_OK) {
      // log_(L_INFO | L_CONS, "Conectado al servidor Redis.\n");

      isConnected = true;
    }
  }

  return isConnected;
}

int redis_init(eredis_t* &e) {
  int status = -1;

  /* eredis */
  e = eredis_new();

  /* conf */
  eredis_host_add(e, redis_host, redis_port);
  eredis_pc_cmd(e, "AUTH %s", redis_pass);

  if (isConnected(e)) {
    status = 0;

    /* run thread */
    eredis_run_thr(e);
  } else {
    log_(L_INFO | L_CONS, "Sin conexion con el servidor Redis.\n");
    eredis_free(e);
    eredis_shutdown(e);
  }

  return status;
}

void redis_close(eredis_t* &e) {
  if (redis_cmd_fail > 0) {
    log_(L_WARN, "Error con eredis_w_cmd %dx\n", redis_cmd_fail);
  }

  log_(L_INFO | L_CONS, "Cerrando conexion... (%d)\n", eredis_w_pending(e));
  /* Let some time to process... normal run... yield a bit... push more write...
   * etc.. */
  while (eredis_w_pending(e) > 0) {
    usleep(1);
  }

  log_(L_DEBUG, "%d) Conexion cerrada.\n", omp_get_thread_num());

  eredis_free(e);
  eredis_shutdown(e);
  e = NULL;
}

void redis_set(char *key, char *value, eredis_t* &e) {
  if (e == NULL) {
    //log_(L_DEBUG, "%d) Redis Init.\n", omp_get_thread_num());
    while (redis_init(e) != 0) {
      sleep(10);
    }
  }

  if (eredis_w_cmd(e, "SET %s %s", key, value) != EREDIS_OK)
    ++redis_cmd_fail;
  else
    ++redis_set_count;

  if (redis_set_count % 10 == 0) {
    /* Let some time to process... normal run... yield a bit... push more
     * write... etc.. */
    while (eredis_w_pending(e) > 0) {
      usleep(U_SLEEP);
    }
  }

  if (redis_set_count % DATA_BLOCK == 0) {
    log_(L_INFO | L_CONS, "Registros cargados: %d\n", redis_set_count);
  }

  // Reconnect from redis.
  /*if (redis_set_count % 5000000 == 0) {
    redis_close();

    if (redis_set_count % 20000000 == 0) {
      sleep(30);  // Wait 30 sec.
    } else {
      sleep(3);  // Wait 3 sec.
    }
  }*/
}

int get_lines(char *filename){
  int lines = 0;
  long phone;
  char value[MAXCHAR];

  FILE *file = fopen(filename, "r");
  if (file == NULL) return -1;

  while (fscanf(file, "%ld|%[^\n]^\n", &phone, value) != EOF) lines++;

  fclose(file);

  return lines;
}

void load_from_file() {
  int lines;
  int phone_count = 0;

  log_(L_INFO | L_CONS, "Cargando registros de %s...\n", filename);

  lines = get_lines(filename);

  //log_(L_DEBUG, "Lineas: %d\n", lines);

  // Thread management.
#pragma omp parallel shared(phone_count)
  {
    static eredis_t *e;
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
          redis_set(key, value, e);
        }
      }
    }

    fclose(file);

    if (isConnected(e)) {
      log_(L_INFO | L_CONS, "Carga completa.\n");
      redis_close(e);
    }
  }

  log_(L_INFO | L_CONS, "Total de registros: %ld\n",
       phone_count);
}

void load_data() {
  FILE *file = fopen(filename, "r");

  if (file != NULL) {
    fclose(file);

    load_from_file();

    log_(L_INFO | L_CONS, "Escuchando cambios en el archivo: %s\n", filename);
  } else {
    log_(L_INFO | L_CONS, "Esperando a la creacion del archivo: %s\n",
         filename);
  }

  log_(L_INFO | L_CONS, "...\n");
}

void file_watcher() {
  // Create a FileWatcher instance that will check the current folder for
  // changes every 5 seconds
  FileWatcher fw{WORKDIR, std::chrono::milliseconds(1000)};

  // Start monitoring a folder for changes and (in case of changes)
  // run a user provided lambda function
  fw.start([](std::string path_to_watch, FileStatus status) -> void {
    // Process only regular files, all other file types are ignored
    if (!std::filesystem::is_regular_file(
            std::filesystem::path(path_to_watch)) &&
        status != FileStatus::erased) {
      return;
    }

    switch (status) {
      case FileStatus::created:
      case FileStatus::modified:
        if (strcmp(path_to_watch.c_str(), filename) == 0) {
          log_(L_INFO | L_CONS, "Archivo creado/modificado: %s\n",
               path_to_watch.c_str());
          load_data();
        }
        break;
      default:
        break;
    }
  });
}

void log_init() {
  char logFilename[MAXCHAR];
  strcpy(logFilename, WORKDIR);
  strcat(logFilename, "/log/redis_data_load.log");

  LOG_CONFIG c = {9, LOG_DEST_FILES, logFilename, "redis_data_load", 0, 1};
  log_set_config(&c);
}

int main(int argc, char *argv[]) {
  fprintf(stderr, "#############################\n");
  fprintf(stderr, "# Data load - Series v%s #\n", VERSION);
  fprintf(stderr, "#############################\n\n");

  log_init();

  if (argc < 5) {
    log_(L_WARN,
         "./redis_data_load.o <FILE_NAME> <REDIS_HOST> <REDIS_PORT> "
         "<REDIS_PASS> <REDIS_DATABASE?>\n");
    exit(1);
  }

  filename = argv[1];
  redis_host = argv[2];
  redis_port = atol(argv[3]);
  redis_pass = argv[4];

  if (argc >= 6) {
    redis_database = atoi(argv[5]);
  }

  signal_conf();

  log_(L_INFO | L_CONS, "Inicializado correctamente.\n");

  load_data();
  file_watcher();

  return 0;
}
