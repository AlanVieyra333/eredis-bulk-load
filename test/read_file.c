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

int main(int argc, char *argv[]) {
  int lines = 0;
  int ignored = 0;
  int phone_count = 0;
  int phone_count_aux = 0;
  long phone_ini, phone_end;
  char value[MAXCHAR];

  if (argc != 2) {
    printf("./read_file.o <FILE_NAME>\n");
    exit(1);
  }

  char *filename = argv[1];
  FILE *file = fopen(filename, "r");

  if (file == NULL) {
    fprintf(stderr, "Could not read file %s\n", filename);
    exit(1);
  }

  for (lines = 0;
       fscanf(file, "%ld|%ld%[^\n]s", &phone_ini, &phone_end, value) != EOF;
       lines++) {
    phone_count_aux += phone_end - phone_ini + 1;

    if (phone_end - phone_ini < 10000) {
      phone_count += phone_end - phone_ini + 1;
    } else {
      ignored++;
    }
  }

  printf("Lineas en el archivo: %d\n", lines);
  printf("Lineas ignoradas: %d\n", ignored);
  printf("Total de registros: %d (%d)\n", phone_count, phone_count_aux);
  printf("Ejemplo de datos en la ultma linea: %ld %ld %s\n", phone_ini,
         phone_end, value);

  fclose(file);

  return 0;
}
