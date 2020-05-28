/**
 * Programa para leer registros de un archivo txt y agregarlos en un servidor
 * Redis.
 * Prueba:
 * Tiempo en cargar 200 millones: 25 min. prox.
 *
 * @todo Al escuchar el cambio de un archivo .dat solo cargar los datos de ese
 * archivo y no cargar nuevamente todos.
 *
 * @version 1.1 Selecciona base de datos dentro de redis.
 * @date 22/05/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
 */

#include <dirent.h>
#include <eredis.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "FileWatcher.h"
#include "log.h"

#define MAXCHAR 300
#define DATA_BLOCK 10000000  // Each DATA_BLOCK reg. reconnect to Redis server.
#define U_SLEEP 100          // Sleep 10us.
#define VERSION 1.1
#define WORKDIR "/data"

char *dirname, *redis_host, *redis_pass;
int redis_port, redis_database = 0;
static eredis_t *e;
int redis_set_count = 0;
int redis_cmd_fail = 0;

struct sigaction old_action;

void sigint_handler(int sig_no) {
  (void)sig_no;

  log_(L_INFO | L_CONS, "CTRL-C pressed\n");
  sigaction(SIGINT, &old_action, NULL);
  if (e) {
    eredis_shutdown(e);
  }
}

void signal_conf() {
  /* to interrupt */
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = &sigint_handler;
  sigaction(SIGINT, &action, &old_action);

  /* cancel sigpipe */
  signal(SIGPIPE, SIG_IGN);
}

bool isConnected() {
  bool isConnected = false;

  if (e) {
    eredis_reply_t *reply; /* aka redisReply from hiredis */
    /* get a reader */
    eredis_reader_t *reader = eredis_r(e);

    reply = eredis_r_cmd(reader, "PING");
    // log_(L_INFO | L_CONS, "PING: %s\n", reply->str);

    /* Release the reader */
    eredis_r_release(reader);

    if (reply != NULL && strcmp(reply->str, "PONG") == 0) {
      // log_(L_INFO | L_CONS, "Conectado al servidor Redis.\n");

      isConnected = true;
    }
  }

  return isConnected;
}

int redis_init() {
  int status = -1;

  /* eredis */
  e = eredis_new();

  /* conf */
  eredis_host_add(e, redis_host, redis_port);
  eredis_pc_cmd(e, "AUTH %s", redis_pass);

  if (isConnected() && eredis_w_cmd(e, "SELECT %d", redis_database) == EREDIS_OK) {
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

void redis_close() {
  if (redis_cmd_fail > 0) {
    log_(L_WARN, "Error con eredis_w_cmd %dx\n", redis_cmd_fail);
  }

  // log_(L_INFO | L_CONS, "Cerrando conexion...\n");
  /* Let some time to process... normal run... yield a bit... push more write...
   * etc.. */
  while (eredis_w_pending(e) > 0) {
    usleep(1);
  }

  eredis_free(e);
  eredis_shutdown(e);
  e = NULL;
}

void redis_set(char *key, char *value) {
  if (!e) {
    while (redis_init() != 0) {
      sleep(10);
    }
  }

  if (eredis_w_cmd(e, "SET %s %s", key, value) != EREDIS_OK)
    ++redis_cmd_fail;
  else
    ++redis_set_count;

  if (redis_set_count % 10000 == 0) {
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
  if (redis_set_count % 5000000 == 0) {
    redis_close();

    if (redis_set_count % 20000000 == 0) {
      sleep(30);  // Wait 30 sec.
    } else {
      sleep(3);  // Wait 3 sec.
    }
  }
}

/**
 * @return Number of lines loaded.
 */
int load_from_file(FILE *file, char *filename) {
  char key[20];
  char value[MAXCHAR];
  int lines;

  if (file == NULL) return 0;

  log_(L_INFO | L_CONS, "Cargando registros de %s ...\n", filename);

  // Lectura de lineas en el archivo.
  fscanf(file, "%d %[^\n]\n", &lines, NULL);
  fscanf(file, "%[^\n]\n", NULL);
  for (int i = 0;
       i < lines && fscanf(file, "%[^~]~%[^\n]\n", key, value) != EOF; i++) {
    /* Cargar a Redis */
    redis_set(key, value);
  }

  log_(L_INFO | L_CONS, "Carga completa. Registros cargados: %ld\n", lines);
  redis_close();

  return lines;
}

bool isDatFile(char const *name) {
  size_t len = strlen(name);
  return len > 4 && strcmp(name + len - 4, ".dat") == 0;
}

void load_data() {
  DIR *d = opendir(dirname);
  int files_count = 0, total_reg = 0;

  log_(L_INFO | L_CONS, "Directorio de archivos billing: %s\n", dirname);

  if (d) {
    struct dirent *dir;

    for (files_count = 0; (dir = readdir(d)) != NULL; files_count++) {
      if (isDatFile(dir->d_name)) {
        char filename[MAXCHAR];
        strcpy(filename, dirname);
        strcat(filename, "/");
        strcat(filename, dir->d_name);
        FILE *file = fopen(filename, "r");

        if (file != NULL) {
          total_reg += load_from_file(file, filename);

          fclose(file);
        } else {
          log_(L_WARN, "Error al abrir el archivo: %s\n", filename);
        }
      }
    }

    log_(L_INFO | L_CONS, "Total de registros cargados: %d\n", total_reg);

    if (files_count == 0) {
      log_(L_INFO | L_CONS,
           "Esperando a la creacion de archivos billing (.dat).\n");
    } else {
      log_(L_INFO | L_CONS, "Escuchando cambios en el directorio: %s\n",
           dirname);
    }

    closedir(d);
  } else {
    log_(L_INFO | L_CONS, "Esperando a la creacion del directorio: %s\n",
         dirname);
  }

  log_(L_INFO | L_CONS, "...\n");
}

void file_watcher() {
  // Create a FileWatcher instance that will check the current folder for
  // changes every second
  FileWatcher fw{dirname, std::chrono::milliseconds(1000)};

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
        if (isDatFile(path_to_watch.c_str())) {
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
  fprintf(stderr, "###########################\n");
  fprintf(stderr, "# Data load - Billing v%.1f #\n", VERSION);
  fprintf(stderr, "###########################\n\n");

  log_init();

  if (argc != 5) {
    log_(L_WARN,
         "./redis_data_load.o <DIR_NAME> <REDIS_HOST> <REDIS_PORT> "
         "<REDIS_PASS> <REDIS_DATABASE?>\n");
    exit(1);
  }

  dirname = argv[1];
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
