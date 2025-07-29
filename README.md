# Sistemas Operativos

Este repositorio contiene los desarrollos realizados para la materia **Sistemas Operativos**, incluyendo tres componentes principales:

## 🐚 Shell
Una implementación básica de una shell que soporta:
- Ejecución de comandos externos
- Comandos encadenados (`|`)
- Redirección de entrada/salida (`>`, `<`)
- Manejo de procesos en background (`&`)

## 🧠 Scheduler
Simulación de un planificador de procesos, con soporte para:
- Algoritmos como Round Robin, FIFO y Prioridades
- Estadísticas de uso de CPU
- Gestión de estados de procesos

## 📁 Filesystem
Implementación de un sistema de archivos simple, con:
- Estructuras tipo i-node
- Soporte para archivos, directorios y permisos básicos
- Lectura/escritura simulada en disco virtual

---

## 🐧 Tecnologías utilizadas

- **Lenguaje:** C
- **Sistema operativo:** Linux
- **Entorno de desarrollo:** Docker
- **Herramientas:** `gcc`, `make`, `gdb`, `valgrind`, `bash`

---

## 🚀 Cómo correr el proyecto

Cada componente del proyecto (shell, scheduler, filesystem) incluye su propio `Makefile` con las instrucciones necesarias para compilar y ejecutar.

Para más detalles, consultá el `README` o los comentarios dentro del `Makefile` correspondiente en cada carpeta:

```bash
cd shell         # o scheduler, filesystem
make             # compila el proyecto
make run         # (si está definido) ejecuta la implementación
