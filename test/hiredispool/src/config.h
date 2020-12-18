#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include <hiredis.h>
#include <async.h>

#include "lib/log.h"
#include "lib/FileWatcher.h"

#define VERSION "1.8.8"
#define MAXCHAR 300
#define WORKDIR "/data"
#define DATA_BLOCK 10000000  // Each DATA_BLOCK reg. reconnect to Redis server.
#define U_SLEEP 5            // Sleep 5us.

std::vector<redisAsyncContext*> c;
const char* filename;

void read_file();

void sigint_handler(int sig_no) {
  /* Disconnects and frees the context */
  for (int i = 0; i < c.size(); i++) {
    redisAsyncDisconnect(c[i]);
  }

  signal(SIGINT, SIG_DFL);
  kill(getpid(), SIGINT);
}

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply *) r;
    fprintf(stderr, "Callback\n");
    if (reply == NULL) return;
    
    log_(L_INFO | L_CONS, "argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        log_(L_ERROR, "Error: %s\n", c->errstr);
        return;
    }

    log_(L_INFO | L_CONS, "Nueva conexion.\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        log_(L_ERROR, "Error: %s\n", c->errstr);
        return;
    }

    log_(L_INFO | L_CONS, "Desconectado.\n");
}

void signal_init() {
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  signal(SIGPIPE, SIG_IGN);
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

void file_watcher_init() {
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

redisAsyncContext *redis_init(const char* hostname, int port, const char* password, int database) {
  redisAsyncContext *c_new = redisAsyncConnect(hostname, port);
  c.push_back(c_new);

  if (c_new->err) {
    /* Let *c leak for now... */
    log_(L_ERROR, "Error: %s\n", c_new->errstr);
    exit(1);
  }

  redisLibevAttach(EV_DEFAULT_ c_new);

  redisAsyncCommand(c_new, NULL, NULL, "AUTH %s", password);
  redisAsyncCommand(c_new, NULL, NULL, "SELECT %d", database);
  redisAsyncSetConnectCallback(c_new, connectCallback);
  redisAsyncSetDisconnectCallback(c_new, disconnectCallback);
  
  return c_new;
}

#endif