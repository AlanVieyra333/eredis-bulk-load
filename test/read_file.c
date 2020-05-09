/**
 * Programa para contar el numero de registros en el archivo Series Siantel -
 * Telcel.
 *
 * @date 08/05/2020
 * @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>
 */

#include <stdio.h>
#include <stdlib.h>

#define MAXCHAR 300

int main() {
  int lines = 0;
  int phone_count = 0;
  long phone_ini, phone_end;
  char value[MAXCHAR];
  char *filename = "seriesSiantel081019.txt";
  FILE *file = fopen(filename, "r");

  if (file == NULL) {
    fprintf(stderr, "Could not read file %s\n", filename);
    exit(1);
  }

  for (lines = 0;
       fscanf(file, "%ld|%ld%[^\n]s", &phone_ini, &phone_end, value) != EOF;
       lines++) {
    phone_count += phone_end - phone_ini + 1;
  }

  printf("Lineas en el archivo: %d\n", lines);
  printf("Total de registros: %d\n", phone_count);
  printf("Ejemplo de datos en la ultma linea: %ld %ld %s\n", phone_ini, phone_end, value);

  fclose(file);

  return 0;
}
