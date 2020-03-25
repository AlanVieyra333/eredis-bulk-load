/**
 * Programa para generar 80 millones de registros en un archivo txt.
 * Tiempo tardado aproximadamente: 35 seg.
 * 
 * @date 23/03/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
*/

#include<stdio.h>
#include<stdlib.h>

#define REG_NUM 50000000

int main() {
    FILE *file = fopen("R09_50millones.txt", "w");

    for (int i = 0; i < REG_NUM; i++)
    {
        fprintf(file, "%s\n", "5554349945|003533863|000000000|GUFE690910QG5 |M0445|MG MASXMENOS 4 12 F |20091015|20080902|+012|FP|BA|PORTA|Identificador de Llamadas|9|");
    }

    fclose(file);

    return 0;
}