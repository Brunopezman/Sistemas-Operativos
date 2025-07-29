#!/bin/bash

# Si se proporciona el flag -p se corre test de persistencia.
if [ "$1" == "-p" ]; then
    chmod +x tests/persistence.sh
    ./tests/persistence.sh
else
    chmod +x tests/archivos_tests.sh
    ./tests/archivos_tests.sh
    chmod +x tests/directorios_tests.sh
    ./tests/directorios_tests.sh
    chmod +x tests/readdir_tests.sh
    ./tests/readdir_tests.sh
    chmod +x tests/write_tests.sh
    ./tests/write_tests.sh
    chmod +x tests/stat_tests.sh
    ./tests/stat_tests.sh
    chmod +x tests/rm_tests.sh
    ./tests/rm_tests.sh
    chmod +x tests/rmdir_tests.sh
    ./tests/rmdir_tests.sh
    chmod +x tests/inodos_limite.sh    
    ./tests/inodos_limite.sh
    echo "=========================================="
    echo "--- GUARDAMOS ARCHIVOS PARA TESTEAR LUEGO LA PERSISTENCIA ---" 
    cd testsdir
    mkdir dir1
    mkdir dir2
    cd dir1
    touch file1
    echo contenido_persistido > file1
    cd ..
    echo "➤ Guardamos dir1,dir2 en testdir..."
    echo "➤ Dentro de 'dir1' guardamos 'file1' con el contenido 'contenido_persistido'..."
    echo "➤ Observamos que se crearon los directorios correctamente..."
    ls
    echo "➤ Observamos que dentro de 'dir1' se encuentra 'file1' con el contenido 'contenido_persistido' y luego lo imprimimos..."
    cd dir1
    ls
    cat file1
    echo "➤ Ejecute los respectivos comandos luego de este script para testear la persistencia"
    cd ..
    cd ..
fi
umount testsdir
rmdir testsdir