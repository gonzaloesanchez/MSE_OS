/*
 * MSE_OS_Core.c
 *
 *  Created on: 26 mar. 2020
 *      Author: gonza
 */

#include "MSE_OS_Core.h"

/*==================[definicion de variables globales]=================================*/

static osControl control_OS;

//----------------------------------------------------------------------------------





/*************************************************************************************************
	 *  @brief Inicializa las tareas que correran en el OS.
     *
     *  @details
     *   Inicializa una tarea para que pueda correr en el OS implementado.
     *   Es necesario llamar a esta funcion para cada tarea antes que inicie
     *   el OS.
     *
	 *  @param *entryPoint		Puntero a la tarea que se desea inicializar.
	 *  @param *task			Puntero a la estructura de control que sera utilizada para
	 *  						la tarea que se esta inicializando.
	 *  @return     None.
***************************************************************************************************/
void os_InitTarea(void *entryPoint, tarea *task)  {
	static uint8_t id = 0;				//el id sera correlativo a medida que se generen mas tareas

	/*
	 * Al principio se efectua un pequeño checkeo para determinar si llegamos a la cantidad maxima de
	 * tareas que pueden definirse para este OS. En el caso de que se traten de inicializar mas tareas
	 * que el numero maximo soportado, se guarda un codigo de error en la estructura de control del OS
	 * y la tarea no se inicializa.
	 */

	if(control_OS.cantidad_Tareas < MAX_TASK_COUNT)  {

		task->stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;					//necesario para bit thumb
		task->stack[STACK_SIZE/4 - PC_REG] = (uint32_t)entryPoint;		//direccion de la tarea (ENTRY_POINT)


		/*
		 * El valor previo de LR (que es EXEC_RETURN en este caso) es necesario dado que
		 * en esta implementacion, se llama a una funcion desde dentro del handler de PendSV
		 * con lo que el valor de LR se modifica por la direccion de retorno para cuando
		 * se termina de ejecutar getContextoSiguiente
		 */
		task->stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;

		task->stack_pointer = (uint32_t) (task->stack + STACK_SIZE/4 - FULL_STACKING_SIZE);

		/*
		 * En esta seccion se guarda el entry point de la tarea, se le asigna id a la misma y se pone
		 * la misma en estado READY. Todas las tareas se crean en estado READY
		 */
		task->entry_point = entryPoint;
		task->id = id;
		task->estado = TAREA_READY;

		/*
		 * Actualizacion de la estructura de control del OS, guardando el puntero a la estructura de tarea
		 * que se acaba de inicializar, y se actualiza la cantidad de tareas definidas en el sistema.
		 * Luego se incrementa el contador de id, dado que se le otorga un id correlativo a cada tarea
		 * inicializada, segun el orden en que se inicializan.
		 */
		control_OS.listaTareas[id] = task;
		control_OS.cantidad_Tareas++;

		id++;
	}

	else {
		control_OS.error = ERR_OS_CANT_TAREAS;		//excedimos la cantidad de tareas posibles
	}
}


/*************************************************************************************************
	 *  @brief Inicializa el OS.
     *
     *  @details
     *   Inicializa el OS seteando la prioridad de PendSV como la mas baja posible. Es necesario
     *   llamar esta funcion antes de que inicie el sistema. Es mandatorio llamarla luego de
     *   inicializar las tareas
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
void os_Init(void)  {
	/*
	 * Todas las interrupciones tienen prioridad 0 (la maxima) al iniciar la ejecucion. Para que
	 * no se de la condicion de fault mencionada en la teoria, debemos bajar su prioridad en el
	 * NVIC. La cuenta matematica que se observa da la probabilidad mas baja posible.
	 */
	NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS)-1);

	/*
	 * Al iniciar el OS se especifica que se encuentra en la primer ejecucion desde un reset.
	 * Este estado es util para cuando se debe ejecutar el primer cambio de contexto. Los
	 * punteros de tarea_actual y tarea_siguiente solo pueden ser determinados por el scheduler
	 */
	control_OS.estado_sistema = OS_FROM_RESET;
	control_OS.tarea_actual = NULL;
	control_OS.tarea_siguiente = NULL;


	/*
	 * El vector de tareas termina de inicializarse asignando NULL a las posiciones que estan
	 * luego de la ultima tarea. Esta situacion se da cuando se definen menos de 8 tareas.
	 * Estrictamente no existe necesidad de esto, solo es por seguridad.
	 */

	for (uint8_t i = 0; i < MAX_TASK_COUNT; i++)  {
		if(i>=control_OS.cantidad_Tareas)
			control_OS.listaTareas[i] = NULL;
	}
}



/*************************************************************************************************
	 *  @brief Funcion que efectua las decisiones de scheduling.
     *
     *  @details
     *   Segun el critero al momento de desarrollo, determina que tarea debe ejecutarse luego, y
     *   por lo tanto provee los punteros correspondientes para el cambio de contexto. Esta
     *   implementacion de scheduler es muy sencilla, del tipo Round-Robin
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
static void scheduler(void)  {
	uint8_t indice;		//variable auxiliar para legibilidad

	/*
	 * El scheduler recibe la informacion desde la variable estado de sistema si es el primer ingreso
	 * desde el ultimo reset. Si esto es asi, determina que la tarea actual es la primer tarea.
	 */
	if (control_OS.estado_sistema == OS_FROM_RESET)  {
		control_OS.tarea_actual = (tarea*) control_OS.listaTareas[0];
	}
	else {

		/*
		 * Obtenemos la siguiente tarea en el vector. Si se llega a la ultima tarea disponible
		 * se hace un reset del contador
		 */
		indice = control_OS.tarea_actual->id+1;
		if(indice < control_OS.cantidad_Tareas)  {
			control_OS.tarea_siguiente = (tarea*) control_OS.listaTareas[indice];
		}
		else  {
			control_OS.tarea_siguiente = (tarea*) control_OS.listaTareas[0];
		}
	}

}


/*************************************************************************************************
	 *  @brief SysTick Handler.
     *
     *  @details
     *   El handler del Systick no debe estar a la vista del usuario. En este handler se llama al
     *   scheduler y luego de determinarse cual es la tarea siguiente a ejecutar, se setea como
     *   pendiente la excepcion PendSV.
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
void SysTick_Handler(void)  {

	/*
	 * Dentro del SysTick handler se llama al scheduler. Separar el scheduler de
	 * getContextoSiguiente da libertad para cambiar la politica de scheduling en cualquier
	 * estadio de desarrollo del OS. Recordar que scheduler() debe ser lo mas corto posible
	 */

	scheduler();

	/**
	 * Se setea el bit correspondiente a la excepcion PendSV
	 */
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

	/**
	 * Instruction Synchronization Barrier; flushes the pipeline and ensures that
	 * all previous instructions are completed before executing new instructions
	 */
	__ISB();

	/**
	 * Data Synchronization Barrier; ensures that all memory accesses are
	 * completed before next instruction is executed
	 */
	__DSB();
}




/*************************************************************************************************
	 *  @brief Funcion para determinar el proximo contexto.
     *
     *  @details
     *   Esta funcion obtiene el siguiente contexto a ser cargado. El cambio de contexto se
     *   ejecuta en el handler de PendSV, dentro del cual se llama a esta funcion
     *
	 *  @param 		sp_actual	Este valor es una copia del contenido de MSP al momento en
	 *  			que la funcion es invocada.
	 *  @return     El valor a cargar en MSP para apuntar al contexto de la tarea siguiente.
***************************************************************************************************/
uint32_t getContextoSiguiente(uint32_t sp_actual)  {
	uint32_t sp_siguiente;


	/*
	 * En la primera llamada a getContextoSiguiente, se designa que la primer tarea a ejecutar sea
	 * la tarea actual, la cual es la primer tarea inicializada y cuyo puntero de estructura fuese
	 * cargado por la funcion scheduler (observar flujo de programa). Como todas las tareas se crean
	 * en estado READY, directamente se cambia a estado RUNNING y se actualiza la variable de estado
	 * de sistema
	 */

	if (control_OS.estado_sistema == OS_FROM_RESET)  {
		sp_siguiente = control_OS.tarea_actual->stack_pointer;
		control_OS.tarea_actual->estado = TAREA_RUNNING;
		control_OS.estado_sistema = OS_NORMAL_RUN;
	}

	/*
	 * En el caso que no sea la primera vez que se ejecuta esta funcion, se hace un cambio de contexto
	 * de manera habitual. Se guarda el MSP (sp_actual) en la variable correspondiente de la estructura
	 * de la tarea corriendo actualmente. Como a este punto no hay mas estados implementados (solamente
	 * READY y RUNNING) se pasa la tarea actual a estado READY.
	 * Se carga en la variable sp_siguiente el stack pointer de la tarea siguiente, que fue definida
	 * por el scheduler. Se actualiza la misma a estado RUNNING y se retorna al handler de PendSV
	 */
	else {
		control_OS.tarea_actual->stack_pointer = sp_actual;
		control_OS.tarea_actual->estado = TAREA_READY;

		sp_siguiente = control_OS.tarea_siguiente->stack_pointer;

		control_OS.tarea_actual = control_OS.tarea_siguiente;
		control_OS.tarea_actual->estado = TAREA_RUNNING;
	}

	return sp_siguiente;
}
