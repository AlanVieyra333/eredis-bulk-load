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

#define MAXCHAR 300

char *filename, *redis_host, *redis_pass;
int redis_port;
static eredis_t *e;
int redis_set_count = 0;
int redis_cmd_fail = 0;

struct sigaction old_action;

void sigint_handler(int sig_no)
{
    (void)sig_no;

    printf("CTRL-C pressed\n");
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
    //printf("PING: %s\n", reply->str);

    /* Release the reader */
    eredis_r_release(reader);

    if (reply != NULL && strcmp(reply->str, "PONG") == 0)
    {
        status = 0;

        /* run thread */
        eredis_run_thr(e);
    } else {
        eredis_free(e);
    }

    return status;
}

void redis_set(char *key, char *value)
{
    //printf("Clave: %s Valor: %s\n", key, value);

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
        printf("Registros cargados: %d\n", redis_set_count);
    }
}

void redis_close()
{
    if (redis_cmd_fail > 0)
    {
        fprintf(stderr, "Error con eredis_w_cmd %dx\n", redis_cmd_fail);
    }

    printf("Cerrando conexión...\n");
    /* Let some time to process... normal run... yield a bit... push more write... etc.. */
    while (eredis_w_pending(e) > 0)
    {
        usleep(10);
    }

    eredis_free(e);
}

void load_from_file()
{
    FILE *file = fopen(filename, "r");
    char line[MAXCHAR];
    char phone[11];

    if (file == NULL)
    {
        fprintf(stderr, "No fue posible abrir el archivo: %s\n", filename);
        return;
    }

    printf("Cargando registros de %s...\n", filename);

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

    printf("Carga completa.\n");

    fclose(file);
}

void load_data()
{
    if (redis_init() == 0)
    {
        load_from_file();
        redis_close();
    }
    else
    {
        printf("Sin conexion con el servidor Redis.\n");
    }

    printf("\nEscuchando cambios en el archivo...\n");
}

void file_watcher()
{
    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{"./", std::chrono::milliseconds(1000)};

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
            if (strcmp(path_to_watch.c_str() + 2, filename) == 0)
            {
                printf("Archivo creado/modificado: %s\n", path_to_watch.c_str());
                load_data();
            }
            break;
        default:
            break;
        }
    });
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "./app.o <FILE_NAME> <REDIS_HOST> <REDIS_PORT> <REDIS_PASS>\n");
        exit(1);
    }

    filename = argv[1];
    redis_host = argv[2];
    redis_port = atoi(argv[3]);
    redis_pass = argv[4];

    signal_conf();

    load_data();
    file_watcher();

    return 0;
}