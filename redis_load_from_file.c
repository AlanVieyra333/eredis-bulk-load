/**
 * Programa para leer 80 millones de registros de un archivo txt.
 * Tiempo tardado aproximadamente: 24.814 seg.
 * 
 * @date 23/03/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
*/

#include <eredis.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

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

void redis_init()
{
    /* to interrupt */
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &sigint_handler;
    sigaction(SIGINT, &action, &old_action);

    /* cancel sigpipe */
    signal(SIGPIPE, SIG_IGN);

    /* eredis */
    e = eredis_new();

    /* conf */
    eredis_host_add( e, redis_host, redis_port );

    eredis_pc_cmd(e, "AUTH %s", redis_pass);

    /* run thread */
    eredis_run_thr(e);
}

void redis_set(char* key, char *value)
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
    printf("Carga completa. Cerrando conexión...\n");
    /* Let some time to process... normal run... yield a bit... push more write... etc.. */
    while (eredis_w_pending(e) > 0)
    {
        usleep(10);
    }

    if (redis_cmd_fail > 0)
    {
        fprintf(stderr, "Error con eredis_w_cmd %dx\n", redis_cmd_fail);
    }

    eredis_free(e);

    if (e)
    {
        eredis_shutdown(e);
    }
}

void load_from_file()
{
    FILE *file = fopen(filename, "r");
    char line[MAXCHAR];
    char phone[11];

    if (file == NULL)
    {
        fprintf(stderr, "Error al abrir el archivo: %s\n", filename);
        exit(1);
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

    fclose(file);
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

    redis_init();
    load_from_file();
    redis_close();

    printf("\nCTRL-C para salir.\n");
    while (1)
    {
        sleep(60*60);
    }

    return 0;
}