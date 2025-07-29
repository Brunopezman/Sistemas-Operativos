# shell

### Búsqueda en $PATH

**Responder: ¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?**

La diferencia entre la syscall `execve(2)` y la familia de wrappers es que la primera, además de ser la interfaz mas básica y directa para reemplazar el proceso actual con un nuevo programa, requiere que se le indique la ruta absoluta del ejecutable. Esta syscall no realiza ninguna búsqueda del ejecutable en las rutas del sistema

Por otro lado, la familia de wrappers proporcionados por la librería estándar de C `exec(3)` esta compuesta por varios wrappers como execl, execle, execlp, execv, execvp, entre otros. Estas funciones ofrecen una interfaz mas amigable al programador con el objetivo de automatizar tareas comunes. Por ejemplo, las variantes que terminan en p (como execvp) realizan una búsqueda del ejecutable en las rutas especificadas en la variable de entorno `$PATH`, facilitando la ejecución de comandos sin necesidad de especificar su ruta absoluta. Todas estas funciones internamente utilizan `execve(2)` como mecanismo final para realizar la ejecucion

**Responder: ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?**

Si, la llamada a `exec(3)` puede fallar por varias razones. Entre las más comunes se encuentran: el archivo especificado no existe, el usuario no tiene permisos de ejecución sobre el archivo, el archivo no es un ejecutable valido, o que el sistema no tenga recursos suficientes para ejecutar un nuevo programa. 

En el caso de que `exec(3)` falle (ya sea porque el ejecutable no existe, no tiene permisos, o no es un binario valido) la función retorna -1. En ese caso, la Shell maneja el error con `perror(“execvp”)`, lo cual imprime un mensaje informativo del sistema indicando la causa del fallo. Posteriormente, el proceso termina con `exit(-1)`.

Por lo tanto, si la ejecución del comando falla, la shell informa explícitamente al usuario mediante un mensaje de error, y termina únicamente el proceso hijo que intentó ejecutar el comando. El proceso padre continúa ejecutándose normalmente y queda a la espera de nuevos comandos, asegurando que la ejecución general de la shell no se vea comprometida.

---

### Procesos en segundo plano
**Explicar detalladamente el mecanismo completo utilizado.**

* Paso 1: Detectar el tipo `BACK` en `run_cmd()`

    - En `run_cmd()` después de parsear el comando, se determina su tipo:
    
        `if (parsed->type == BACK)`
    
        Eso significa que la línea ingresada por el usuario terminó con &.

* Paso 2: Crear un grupo de procesos para los procesos en background

    Para mantener organizados los procesos en segundo plano, se les asigna un Process Group ID (PGID) común.
    - Si es el primer proceso en segundo plano:
        * Se usa su propio PID como PGID.
        * Se guarda ese PGID globalmente en pgid_back para próximos procesos.

            ```
            if (pgid_back == -1) {
                setpgid(p, p);
                pgid_back = p;
            }
            ```

    - Si no es el primero: Se asigna al grupo ya existente.

        ```
        else {
            setpgid(p, pgid_back);
        }
        ```

	Así, todos los procesos en background comparten ese pgid_back.

* Paso 3: Notificar que el proceso está en background

	Se imprime información del proceso en segundo plano usando:
	
	`print_back_info(parsed);`
	
	Para que el usuario vea algo como: 
	
	`[PID=8671]`

* Paso 4: No esperar a que termine (a diferencia del foreground)

    En lugar de llamar a waitpid() que bloquearía la shell, se deja correr y se devuelve el control al usuario.

    Solo si es foreground (else en run_cmd), se hace:
	
    ```
	waitpid(p, &status, 0);
    print_status_info(parsed);
    ```
    
    Pero no en el caso BACK.

* Paso 5: Configurar SIGCHLD para detectar procesos terminados
    En run_shell():

    * Se instala un manejador de señal SIGCHLD con sigaction.
    * Cada vez que un hijo termina, se llama sigchld_handler().

    ```
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    ```

* Paso 6: sigchld_handler() se encarga de los procesos finalizados

    Cuando algún proceso en background termina:

    1.	waitpid(-pgid_back, &status, WNOHANG) revisa los procesos del grupo.
        * El -pgid_back indica “cualquier proceso dentro del grupo”.
        * WNOHANG evita bloquear, sólo informa si terminó alguno.
    2.	Si uno terminó:
        * Se imprime mensaje de finalización:
        
        `snprintf(str, sizeof str, "\r==> terminado: PID=%d\n", pid);`
        * Se actualiza prompt para el usuario.

    3.	Luego se revisa si quedan procesos en ese grupo:

        ```
        pid = waitpid(-pgid_back, &status, WNOHANG);
        if (pid == -1 && errno == ECHILD) {
            pgid_back = -1;
        }
        ```

        Si no queda ninguno, se limpia pgid_back para permitir nuevos grupos.
        
**Responder:¿Por qué es necesario el uso de señales?**

Las señales son necesarias porque porque sin ellas, la shell no se entera cuándo terminan los procesos en segundo plano y se evita que queden zombies ocupando espacio en la tabla de procesos. La shell no lo espera al proceso con `waitpid()`, así que necesita usar una señal para enterarse que terminó. Esa señal es: `SIGCHLD`. Cuando un hijo termina, el sistema le envía al padre un `SIGCHLD`. La shell puede atrapar esa señal con un handler y entonces ahi llamar a `waitpid()`.

---

### Flujo estándar

**Investigar el significado de 2>&1, explicar cómo funciona su forma general**

En la línea de comandos de sistemas Unix/Linux, 2>&1 toma el error estándar (stderr) y lo redirige al mismo destino que la salida estándar (stdout).
* `2`: Representa el descriptor de archivo para el error estándar (stderr). 
* `>`: Es el operador de redirección, que indica que el flujo de un descriptor de archivo se debe redirigir a otro. 
* `&1`: Significa "el descriptor de archivo que está actualmente asignado a la salida estándar (stdout)"

**1. Mostrar qué sucede con la salida de cat out.txt en el ejemplo.**

![alt text](/images/cat_out_shell.png)

**2. Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).**

![alt text](/images/cat_out2_shell.png)

Se observa que en nuestra shell no hay cambios sea cual sea orden de las redirecciones. Para entender esto hay que observar que ocurre en la funcion parse_redir_flow dentro de parsing.c. Tanto la salida estándar (stdout) como la salida de error estándar (stderr) están siendo redirigidas al mismo archivo en el primer comando, en el que primero se redirige stodut al archivo "out.txt" y luego se redirige la stderr a la misma dirección que stdout (es decir, al archivo "out.txt"). Cuando se invierte el orden, stdin se redirige primero a stdout, y luego stdin y stdout redirigen ambos a "out.txt".

Esto ocurre en bash:

![error y out.txt en misma salida](/images/cat_out_bash.png)

![error y out.txt por distintas salidas](/images/cat_out2_bash.png)

Podemos observar que en el primer caso tanto el Listado de `/home` y el mensaje de error por `/noexiste`. Sin embargo, en el segundo caso, solo se ve el listado de /home (el error se imprimió en la pantalla cuando se ejecutó el comando).

--- 

### Tuberías múltiples

**Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo?**
**¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.**

Cuando se ejecuta un pipe en una shell, el exit code reportado al final corresponde al último comando del pipeline. Es decir, aunque un comando anterior en el pipeline falle, si el último comando tiene un código de salida 0 (éxito), entonces el exit code del pipeline completo será 0.

Ejemplo de pipe sin error al final:

![alt text](/images/pipe_false_true.png)

Ejemplo de pipe con error al final:

![alt text](/images/pipe_true_false.png)

En cambio, en nuestra shell, no importa lo que ocurra con el ultimo comando del pipeline, siempre el codigo de salida sera 0 debido a que no contemplamos el estado de la salida particular del ultimo comando.

![alt text](/images/pipe_shell.png)

---

### Variables de entorno temporarias

**Responder: ¿Por qué es necesario hacerlo luego de la llamada a `fork(2)`?**

Es necesario hacerlo luego de la llamada a `fork(2)` porque las modificaciones al entorno realizadas con funciones como `setenv(3)` afectan únicamente al proceso actual. Cuando se llama a `fork(2)`, se crea un nuevo proceso hijo que hereda una copia del entorno del padre. Si se desea que las variables de entorno temporarias afecten únicamente al proceso hijo y no al padre ni a otros procesos, es necesario aplicar los cambios luego del fork, dentro del hijo. De este modo, el entorno modificado se utiliza únicamente para la ejecución del comando deseado, sin alterar el entorno global del shell.

**En algunos de los wrappers de la familia de funciones de `exec(3)` (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`. ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.**

No, el comportamiento no es exactamente el mismo. Cuando se utilizan funciones como `setenv(3)` luego del `fork(2)`, se modifica el entorno del proceso hijo directamente antes de llamar a `exec(3)`, y ese entorno se mantiene hasta que el proceso termine o se reemplace. En cambio, al pasar un arreglo de variables al tercer argumento de una función como `execle(3)` o `execve(2)`, se omite completamente el entorno heredado del proceso padre: el nuevo proceso reemplaza su entorno por el arreglo pasado. Esto significa que, a menos que ese arreglo contenga explícitamente todas las variables necesarias (como `PATH`, `HOME`, etc.), podrían perderse variables importantes, generando diferencias en el comportamiento.

**Responder: Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.**

Una posible implementación para mantener el mismo comportamiento sería utilizar un nuevo arreglo de entorno que contenga todas las variables actuales del entorno, y luego aplicar las modificaciones o adiciones temporarias que se deseen. 

---

### Pseudo-variables

**Responder: Investigar al menos otras tres variables mágicas estándar, y describir su propósito.**
**- Incluir un ejemplo de su uso en bash (u otra terminal similar).**

1. `$0`: nombre de script o comando ejecutado.
Esta variable contiene el nombre del script que está siendo ejecutado, tal como fue invocado en la línea de comandos. Es útil para mostrar el nombre del programa dentro del mismo script.

Ejemplo de uso:

Si en un archivo llamado `mi_script.sh` se guarda la siguiente línea: `echo “Nombre del script: $0”` y se ejecuta `./mi_script` se imprimirá: `Nombre del script: ./mi_script.sh`

2. `$#`: números de argumentos pasados al script.

Esta variable almacena la cantidad total de argumentos que se le pasan al script o función en el momento de su ejecución. Es útil para verificar que el número de argumentos sea el esperado.

Ejemplo de uso:

Si en un archivo llamado mi_script.sh se guarda la siguiente línea: `echo “Se pasaron $# argumentos al script.”` y se ejecuta `./mi_script.sh arg1 arg2 arg3`, se imprimirá: `Se pasaron 3 argumentos al script.`

3. `$@`: lista de todos los argumentos.
Esta variable representa todos los argumentos pasados al script, como una lista. Se suele usar en bucles para procesar todos los argumentos individualmente.

Ejemplo de uso:

Si en un archivo llamado `mi_script.sh` se guarda la siguiente línea:

    ```
    for arg in `"$@"`; do
        echo "Argumento: $arg"
    done
    ```

y se ejecuta `./mi_script.sh` a b c, se imprimirá:

    ```
    Argumento: a
    Argumento: b
    Argumento: c
    ```

---

### Comandos built-in
**¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)**

Un comando built-in es aquel que está implementado directamente dentro del programa de la shell, en lugar de ser un programa externo que se ejecuta con `exec()`. Se implentan asi porque algunos comandos necesitan afectar el entorno de la propia shell, y eso no se puede hacer desde un proceso hijo.

`cd` DEBE ser un comando built-in. Si lo implementaras como un binario externo (es decir, que se ejecute en un `fork()`): Cambiaría su directorio de trabajo con `chdir()` y Terminaría. Al volver al padre, no pasaría nada, porque cada proceso tiene su propio cwd. Por ello conviene que se implemente como built-in.

`pwd` puede no ser un built-in porque simplemente imprime el directorio actual con `getcwd`. No cambia nada del entorno. Pero, conviene que lo sea dado que ejecutarlo directamente desde la shell, evita el uso de `fork()` y `exec()` para mayor eficiencia.

---

### Historial

---
