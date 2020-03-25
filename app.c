#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <eredis.h>

static eredis_t *e;

int main(int argc, char *argv[])
{
    /* optional command line arguments */
    const char *host_file = "hosts.conf";
    if (argc >= 2)
    {
        host_file = argv[1];
    }

    /* eredis */
    e = eredis_new();

    /* conf */
    if (eredis_host_file(e, host_file) <= 0)
    {
        fprintf(stderr, "Unable to load conf %s\n", host_file);
        exit(1);
    }

    eredis_pc_cmd(e, "AUTH %s", "TeLcEl");

    /* run thread */
    eredis_run_thr(e);

    int fail = 0;
    char fmt[300];
    for (int i = 0; i <= 1000000*50; i++)
    {
        sprintf(fmt, "5255%d", i);
        if (eredis_w_cmd(e, "SET %s %i", fmt, i) != EREDIS_OK)
            ++fail;
    }

    if (fail > 0)
    {
        fprintf(stderr, "Failed to eredis_w_cmd %dx\n", fail);
    }

    /* Let some time to process... normal run... yield a bit... push more write... etc.. */
    printf("Async running...\n");
    while (eredis_w_pending(e) > 0)
    {
        usleep(1000);
    }

    eredis_free(e);

    if (e)
    {
        eredis_shutdown(e);
    }

    // printf("Carga completa.\n\nCTRL-C when you want to exit\n");
    // while (1)
    // {
    //     sleep(60);
    // }

    return 0;
}