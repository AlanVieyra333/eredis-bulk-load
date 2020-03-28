/**
 * Programa para leer 80 millones de registros de un archivo txt.
 * Tiempo tardado aproximadamente: 24.814 seg.
 * 
 * @date 23/03/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <eredis.h>
#include "FileWatcher.h"
#include "log.h"

#define MAXCHAR 300

char *filename, *redis_host, *redis_pass;
char *workdir = "/data";
int redis_port;
static eredis_t *e;
int redis_set_count = 0;
int redis_cmd_fail = 0;

struct sigaction old_action;

void sigint_handler(int sig_no)
{
    (void)sig_no;

    log_(L_INFO | L_CONS, "CTRL-C pressed\n");
    sigaction(SIGINT, &old_action, NULL);
    if (e)
    {
        eredis_shutdown(e);
    }
}

void signal_conf()
{
    /* to interrupt */
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    /* cancel sigpipe */
    signal(SIGPIPE, SIG_IGN);
}

int redis_init()
{
    int status = -1;

    /* eredis */
    e = eredis_new();

    /* conf */
    eredis_host_add(e, redis_host, redis_port);

    eredis_pc_cmd(e, "AUTH %s", redis_pass);

    eredis_reply_t *reply; /* aka redisReply from hiredis */
    /* get a reader */
    eredis_reader_t *reader = eredis_r(e);

    reply = eredis_r_cmd(reader, "PING");
    //log_(L_INFO | L_CONS, "PING: %s\n", reply->str);

    /* Release the reader */
    eredis_r_release(reader);

    if (reply != NULL && strcmp(reply->str, "PONG") == 0)
    {
        log_(L_INFO | L_CONS, "Conexion establecida con el servidor Redis.\n");

        status = 0;

        /* run thread */
        eredis_run_thr(e);
    }
    else
    {
        eredis_free(e);
    }

    return status;
}

void redis_set(char *key, char *value)
{
    //log_(L_INFO | L_CONS, "Clave: %s Valor: %s\n", key, value);

    if (eredis_w_cmd(e, "SET %s %s", key, value) != EREDIS_OK)
        ++redis_cmd_fail;
    else
        ++redis_set_count;

    if (redis_set_count % 100000 == 0)
    {
        /* Let some time to process... normal run... yield a bit... push more write... etc.. */
        while (eredis_w_pending(e) > 0)
        {
            usleep(10);
        }
    }

    if (redis_set_count % 10000000 == 0)
    {
        log_(L_INFO | L_CONS, "Registros cargados: %d\n", redis_set_count);
    }
}

void redis_close()
{
    if (redis_cmd_fail > 0)
    {
        log_(L_WARN, "Error con eredis_w_cmd %dx\n", redis_cmd_fail);
    }

    log_(L_INFO | L_CONS, "Cerrando conexion...\n");
    /* Let some time to process... normal run... yield a bit... push more write... etc.. */
    while (eredis_w_pending(e) > 0)
    {
        usleep(10);
    }

    eredis_free(e);
}

void load_from_file(FILE *file)
{
    char line[MAXCHAR];
    char phone[11];

    if (file == NULL)
        return;

    log_(L_INFO | L_CONS, "Cargando registros de %s...\n", filename);

    while (fgets(line, MAXCHAR, file) != NULL)
    {
        //sscanf(line, "%ld", &phone);
        for (char i = 0; i < 10; i++)
        {
            phone[i] = line[i];
        }

        phone[10] = '\0';

        /* Cargar a Redis */
        redis_set(phone, line);
    }

    log_(L_INFO | L_CONS, "Carga completa.\n");
}

void load_data()
{
    FILE *file = fopen(filename, "r");

    if (file != NULL)
    {
        if (redis_init() == 0)
        {
            load_from_file(file);
            redis_close();
        }
        else
        {
            log_(L_INFO | L_CONS, "Sin conexion con el servidor Redis.\n");
        }

        fclose(file);

        log_(L_INFO | L_CONS, "Escuchando cambios en el archivo: %s\n", filename);
    } else {
        log_(L_INFO | L_CONS, "Esperando a la creacion del archivo: %s\n", filename);
    }

    log_(L_INFO | L_CONS, "...\n");
}

void file_watcher()
{
    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{workdir, std::chrono::milliseconds(1000)};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([](std::string path_to_watch, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        if (!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased)
        {
            return;
        }

        switch (status)
        {
        case FileStatus::created:
        case FileStatus::modified:
            if (strcmp(path_to_watch.c_str(), filename) == 0)
            {
                log_(L_INFO | L_CONS, "Archivo creado/modificado: %s\n", path_to_watch.c_str());
                load_data();
            }
            break;
        default:
            break;
        }
    });
}

void log_init()
{
    char logFilename[MAXCHAR];
    strcpy(logFilename, workdir);
    strcat(logFilename, "/log/redis_load_from_file.log");

    LOG_CONFIG c = {
        9,
        LOG_DEST_FILES,
        logFilename,
        "redis_load_from_file",
        0,
        1};
    log_set_config(&c);
}

int main(int argc, char *argv[])
{
    fprintf(stderr, "###################\n");
    fprintf(stderr, "# Redis bulk load #\n");
    fprintf(stderr, "###################\n\n");

    log_init();

    if (argc != 5)
    {
        log_(L_WARN, "./app.o <FILE_NAME> <REDIS_HOST> <REDIS_PORT> <REDIS_PASS>\n");
        exit(1);
    }

    filename = argv[1];
    redis_host = argv[2];
    redis_port = atoi(argv[3]);
    redis_pass = argv[4];

    signal_conf();

    log_(L_INFO | L_CONS, "Inicializado correctamente.\n");

    load_data();
    file_watcher();

    return 0;
}