#!/bin/bash
FILENAME=test12.in
CORE=4
DAMP=0.85
NUMN=1000
NUME=3000


echo "${CORE}" > "${FILENAME}"
echo "${DAMP}" >> "${FILENAME}"

echo "${NUMN}" >> "${FILENAME}"
for ((i=1;i<=$NUMN;i++));
do
    echo "${i}" >> "${FILENAME}"
done

echo "${NUME}" >> "${FILENAME}"

for ((i=1;i<=$NUME;i++));
do
    echo "$((1 + $RANDOM % ($NUMN))) $((1 + $RANDOM % $NUMN))" >> "${FILENAME}"
done