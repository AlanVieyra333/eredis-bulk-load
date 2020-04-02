#!/bin/bash
# Programa para para leer 80 millones de registros de un archivo txt.
# Tiempo tardado aproximadamente: 9 min.
# 
# @date 23/03/2020
# @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>

FILE_NAME="R09_80millones.txt"
i=0

while IFS= read -r line
do
  #echo "$line"
  i=$((i + 1))

done < "$FILE_NAME"

echo "Lines: " $i