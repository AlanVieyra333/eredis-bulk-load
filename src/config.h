#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>
#include <vector>

#include <hiredis.h>

#include "lib/log.h"
#include "lib/FileWatcher.h"

#define VERSION "1.8.9"
#define MAXCHAR 300
#define WORKDIR "/data"
#define REDIS_BLOCK 100
#define DATA_BLOCK 10000000  // Each DATA_BLOCK reg. reconnect to Redis server.

std::vector<redisContext*> c;
const char* filename;

void read_file();

void sigint_handler(int sig_no) {
  /* Disconnects and frees the context */
  for (int i = 0; i < c.size(); i++) {
    redisFree(c[i]);
  }

  signal(SIGINT, SIG_DFL);
  kill(getpid(), SIGINT);
}

void signal_init() {
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  //signal(SIGPIPE, SIG_IGN);
}

void log_init(){
  char logFilename[MAXCHAR];
  strcpy(logFilename, WORKDIR);
  strcat(logFilename, "/log/redis_data_load.log");

  LOG_CONFIG log_conf = {9, LOG_DEST_FILES, logFilename, "redis_data_load", 0, 1};
  log_set_config(&log_conf);
}

void file_check(const char* filename) {
  FILE *file = fopen(filename, "r");

  if (file != NULL) {
    fclose(file);

    read_file();

    log_(L_INFO | L_CONS, "Escuchando cambios en el archivo: %s\n", filename);
  } else {
    log_(L_INFO | L_CONS, "Esperando a la creacion del archivo: %s\n",
        filename);
  }

  log_(L_INFO | L_CONS, "...\n");
}

void file_watcher_start() {
  file_check(filename);
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
          file_check(filename);
        }
        break;
      default:
        break;
    }
  });
}

redisContext *redis_init(const char* hostname, int port, const char* password, int database) {
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  redisContext *c_new = redisConnectWithTimeout(hostname, port, timeout);
  redisReply *reply;
  
  if (c_new == NULL || c_new->err) {
    if (c_new) {
      log_(L_ERROR, "Connection error: %s\n", c_new->errstr);
      redisFree(c_new);
    } else {
      log_(L_ERROR, "Connection error: can't allocate redis context\n");
    }

    exit(1);
  }

  c.push_back(c_new);

  reply = (redisReply *) redisCommand(c_new,"AUTH %s", password);
  if (strcmp(reply->str, "OK") == 0) {
    log_(L_INFO | L_CONS, "[%d] Conectado.\n", omp_get_thread_num());
  } else {
    log_(L_WARN | L_CONS, "[%d] No conectado.\n", omp_get_thread_num());
  }
  freeReplyObject(reply);

  reply = (redisReply *) redisCommand(c_new,"SELECT %d", database);
  // log_(L_INFO | L_CONS, "SELECT: %s\n", reply->str);
  freeReplyObject(reply);
  
  return c_new;
}

#endif