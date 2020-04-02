/**
 * Programa para leer 80 millones de registros de un archivo txt.
 * Tiempo tardado aproximadamente: 24.814 seg.
 * 
 * @date 23/03/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
*/

#include<stdio.h>
#include<stdlib.h>

#define MAXCHAR 300

char line[MAXCHAR];

int main() {
    FILE *file = fopen("R09_80millones.txt", "r");
    long phone;

    while (fgets(line, MAXCHAR, file) != NULL)
    {
        sscanf(line, "%ld", &phone);
        
        /* Cargar a Redis */
    }

    fclose(file);

    return 0;
}