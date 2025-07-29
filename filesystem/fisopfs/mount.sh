#!/bin/bash

make clean
# Si se proporciona el flag -p no se intenta borrar el archivo.
if [ "$1" == "-p" ]; then
    echo "corriendo fisopfs y levantando datos persistidos"
else
    # si existe el archivo, lo elimina
    if [ -f persistence_file.fisopfs ]; then
        rm persistence_file.fisopfs
    fi
fi
mkdir testsdir
make
./fisopfs -f testsdir
