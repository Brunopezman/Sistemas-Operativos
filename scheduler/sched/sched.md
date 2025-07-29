# Sched

## 1- Cambio de contexto: 

En esta sección vamos a utilizar GDB para visualizar el cambio de contexto. Para ello mostraremos capturas donde se visualice efectivamente:
    
* El cambio de contexto
* El estado del stack al inicio de la llamada de context_switch
* Cómo cambia el stack instrucción a instrucción
* Cómo se modifican los registros luego de ejecutar iret


Primeramente iniciamos GDB y mostramos el estado de todos los registros al inicio de "context_switch"...

![estado inicial registros](/img/cambio_contexto1.png)

Ahora iremos avanzando paso a paso, viendo los estados de los registros instrucción a instrucción, hasta finalmente llegar al llamado de "iret"... 

![instrucción a instrucción](/img/cambio_contexto2.png)

Siguiente instrucción...

![instrucción a instrucción](/img/cambio_contexto3.png)

Siguiente instrucción...

![instrucción a instrucción](/img/cambio_contexto4.png)

Siguiente instrucción...

![instrucción a instrucción](/img/cambio_contexto5.png)

Siguiente instrucción...

![instrucción a instrucción](/img/cambio_contexto6.png)

Siguiente instrucción...

![instrucción a instrucción](/img/cambio_contexto7.png)

Finalmente, al avanzar una última vez para mostrar los estados, notamos efectivamente los cambios en los registros -> "esp", "eip", "cs".

![muestra final registros](/img/cambio_contexto8.png)

Como agregado, sumamos una muestra que el programa efectivamente se finaliza de manera correcta al salir de GDB, ejecutándose bien el "hello.c"...

![finalización exitosa programa](/img/cambio_contexto9.png)

## 2- Scheduler con prioridades: 

En esta seccion se modifico el scheduler para que considere prioridades al momento de seleccionar qué proceso ejecutar. Se definió un esquema de prioridades numéricas donde un número menor representa una mayor prioridad (por ejemplo, 0 es la prioridad más alta y 10 la más baja). Todos los procesos son creados con una prioridad por defecto de 5, valor definido por una constante. Para almacenar esta información, se agregó un nuevo campo priority.

El scheduler fue modificado en la función sched_yield para que elija siempre al proceso ENV_RUNNABLE con menor prioridad entre todos los entornos disponibles. Si varios procesos tienen la misma prioridad, se elige el primero que aparece en el recorrido circular a partir del proceso actual. En caso de que no haya procesos RUNNABLE, pero el proceso actual aún se encuentre en estado RUNNING, se lo sigue ejecutando.

Además, se implementaron dos nuevas syscalls para trabajar con prioridades desde espacio de usuario. La syscall sys_get_priority permite a un proceso consultar su prioridad actual, mientras que la syscall sys_set_priority le permite modificarla bajo ciertas condiciones. Para garantizar la seguridad y evitar que un proceso se favorezca injustamente, se impuso una restricción: un proceso sólo puede reducir su prioridad (es decir, aumentar su valor numérico), nunca aumentarla. Si un proceso intenta establecer una prioridad fuera del rango permitido o superior a la actual (menor valor), la syscall devuelve un error.

 
Para comprobar el correcto funcionamiento del sistema de prioridades, se desarrollo un proceso de usuario que realiza varias pruebas que intentan modificar su prioridad en tiempo de ejecucion.

![ejemplo 1 con prioridades](/img/prueba_prioridad1.png)

El proceso comienza con la prioridad por defecto (valor 5), lo cual se verifica utilizando la syscall sys_get_priority. A continuacion, se intenta mejorar la prioridad, es decir, asignarle un valor mas alto en jerarquía (numericamente menor, en este caso 2). Como se espera, la syscall devuelve un error, ya que esta prohibido que un proceso aumente su propia prioridad. Mediante una nueva invocacion a sys_get_priority, se confirma que la prioridad del proceso permanece sin cambios.

Seguidamente, se intenta reducir la prioridad asignandole el valor 10, lo cual sí esta permitido. La syscall se ejecuta correctamente, y una nueva consulta con sys_get_priority verifica que el valor fue efectivamente actualizado.

Finalmente, se realiza un nuevo intento de mejorar la prioridad (por ejemplo, volver a 5), el cual tambien falla como se espera, y la prioridad se mantiene en 10.

Esto confirma que:
* Un proceso no puede aumentar su propia prioridad (asignar un valor numérico menor al actual).
* Un proceso sí puede reducir su prioridad (asignar un valor numérico mayor).
* El scheduler respeta la lógica de selección basada en la prioridad más alta (número menor)


Por otro lado, cuando un proceso crea un hijo mediante fork, este hereda la misma prioridad del padre. 

![ejemplo 2 con prioridades](/img/prueba_prioridad2.png)

Este test demuestra que:
* El sistema impide aumentar la prioridad de un proceso.
* Es posible reducir la prioridad.
* Los procesos hijos heredan correctamente la prioridad del padre.





