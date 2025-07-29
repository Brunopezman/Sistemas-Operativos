// hello, world
#include <inc/lib.h>

// ************** TEST ORIGINAL ************** //

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
}

// ************** Prueba de setear prioridad ************** //

// void
// umain(int argc, char **argv)
// {
// 	envid_t me = sys_getenvid();
// 	int r;
// 	int priority;
// 	cprintf("********** Inicio del test **********\n\n");
// 	cprintf("Soy el proceso %08x\n", thisenv->env_id);
// 	cprintf("La prioridad de este proceso es %d\n\n", sys_get_priority(me));

// 	cprintf("- Se intenta incrementar la prioridad a 2...\n");
// 	int prioridad1 = sys_set_priority(me,2);
// 	if (prioridad1 < 0) {
// 		cprintf("Error al setear la prioridad del proceso\n\n");
// 	}

// 	cprintf("La prioridad de este proceso es: %d\n\n", sys_get_priority(me));

// 	cprintf("- Se intenta decrementar la prioridad del proceso a 10\n");

// 	int prioridad2 = sys_set_priority(me, 10);
// 	if (prioridad2 < 0) {
// 		cprintf("Error al setear la prioridad del proceso\n");
// 	}

// 	cprintf("La prioridad de este proceso ahora es: %d\n\n", sys_get_priority(me));

// 	cprintf("- Se intenta incremetar la prioridad del proceso a 7\n");

// 	int prioridad3 = sys_set_priority(me, 7);
// 	if (prioridad3 < 0) {
// 		cprintf("Error al setear la prioridad del proceso\n");
// 	}

// 	cprintf("La prioridad de este proceso ahora es: %d\n\n", sys_get_priority(me));

// 	cprintf("********** Fin del test **********\n\n");
// }

// ************** PRUEBA USANDO fork ************** //

// void
// umain(int argc, char **argv)
// {
// 	envid_t me = sys_getenvid();
// 	cprintf("********** Inicio del test **********\n\n");

// 	cprintf("[Padre] Soy el proceso %08x\n", thisenv->env_id);
// 	cprintf("[Padre] La prioridad inicial es %d\n\n", sys_get_priority(me)); // Empieza con 5

// 	cprintf("[Padre] Intento aumentar mi prioridad a 3...\n");
// 	if (sys_set_priority(me, 3) < 0) {
// 		cprintf("[Padre] Error al setear la prioridad\n");
// 	} else {
// 		cprintf("[Padre] Prioridad actual: %d\n\n", sys_get_priority(me));
// 	}

// 	cprintf("[Padre] Seteando prioridad a 7...\n");
// 	if (sys_set_priority(me, 7) < 0) {
// 		cprintf("[Padre] Error al setear la prioridad\n");
// 	} else {
// 		cprintf("[Padre] Prioridad actual: %d\n\n", sys_get_priority(me));
// 	}

// 	envid_t hijo = fork();
// 	if (hijo < 0) {
// 		cprintf("Error al hacer fork\n");
// 	} else if (hijo == 0) {
// 		// Código del hijo
// 		envid_t yo = sys_getenvid();
// 		cprintf("[Hijo] Soy el proceso %08x\n", thisenv->env_id);
// 		cprintf("[Hijo] Prioridad heredada/inicial: %d\n", sys_get_priority(yo));

// 		cprintf("[Hijo] Seteando prioridad a 8...\n");
// 		if (sys_set_priority(yo, 8) < 0) {
// 			cprintf("[Hijo] Error al setear la prioridad\n");
// 		} else {
// 			cprintf("[Hijo] Nueva prioridad: %d\n", sys_get_priority(yo));
// 		}

// 		cprintf("[Hijo] Intento aumentar mi prioridad a 5...\n");
// 		if (sys_set_priority(yo, 5) < 0) {
// 			cprintf("[Hijo] Error al setear la prioridad\n");
// 		} else {
// 			cprintf("[Hijo] Nueva prioridad: %d\n", sys_get_priority(yo));
// 		}
// 	} else {
// 		// Código del padre
// 		sys_yield(); // Ceder CPU para que el hijo corra primero
// 		cprintf("\n[Padre] Volviendo a correr. Soy %08x\n", thisenv->env_id);
// 		cprintf("[Padre] Mi prioridad sigue siendo: %d\n", sys_get_priority(me));
// 		cprintf("\n********** Fin del test **********\n\n");
// 	}

// }