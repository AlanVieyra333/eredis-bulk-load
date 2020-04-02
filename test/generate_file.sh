#!/bin/bash
# Programa para para generar 80 millones de registros en un archivo txt.
# Tiempo tardado aproximadamente: 30 min.
# 
# @date 23/03/2020
# @author Alan Fernando Rincón Vieyra <alan.rincon@mail.telcel.com>

FILE_NAME="R09_80millones.txt"
REG_NUM=80000000

for (( i=1; i<=$REG_NUM; i++ ))
do
   echo "5554349945|003533863|000000000|GUFE690910QG5 |M0445|MG MASXMENOS 4 12 F |20091015|20080902|+012|FP|BA|PORTA|Identificador de Llamadas|9|" >> ${FILE_NAME}
done
