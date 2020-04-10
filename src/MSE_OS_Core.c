/*
 * MSE_OS_Core.c
 *
 *  Created on: 26 mar. 2020
 *      Author: gonza
 */

#include "MSE_OS_Core.h"






/*==================[definicion de variables globales]=================================*/

static osControl control_OS;
static tarea tareaIdle;

//----------------------------------------------------------------------------------

/*==================[definicion de prototipos static]=================================*/
static void initTareaIdle(void);
static void setPendSV(void);


/*==================[definicion de hooks debiles]=================================*/

/*
 * Esta seccion contiene los hooks de sistema, los cuales el usuario del OS puede
 * redefinir dentro de su codigo y poblarlos segun necesidad
 */


/*************************************************************************************************
	 *  @brief Hook de retorno de tareas
     *
     *  @details
     *   Esta funcion no deberia accederse bajo ningun concepto, porque ninguna tarea del OS
     *   debe retornar. Si lo hace, es un comportamiento anormal y debe ser tratado.
     *
	 *  @param none
	 *
	 *  @return none.
***************************************************************************************************/
void __attribute__((weak)) returnHook(void)  {
	while(1);
}



/*************************************************************************************************
	 *  @brief Hook de tick de sistema
     *
     *  @details
     *   Se ejecuta cada vez que se produce un tick de sistema. Es llamada desde el handler de
     *   SysTick.
     *
	 *  @param none
	 *
	 *  @return none.
	 *
	 *  @warning	Esta funcion debe ser lo mas corta posible porque se ejecuta dentro del handler
     *   			mencionado, por lo que tiene prioridad sobre el cambio de contexto y otras IRQ.
	 *
	 *  @warning 	Esta funcion no debe bajo ninguna circunstancia utilizar APIs del OS dado
	 *  			que podria dar lugar a un nuevo scheduling.
***************************************************************************************************/
void __attribute__((weak)) tickHook(void)  {
	__asm volatile( "nop" );
}



/*************************************************************************************************
	 *  @brief Hook de error de sistema
     *
     *  @details
     *   Esta funcion es llamada en caso de error del sistema, y puede ser utilizada a fin de hacer
     *   debug. El puntero de la funcion que llama a errorHook es pasado como parametro para tener
     *   informacion de quien la esta llamando, y dentro de ella puede verse el codigo de error
     *   en la estructura de control de sistema. Si ha de implementarse por el usuario para manejo
     *   de errores, es importante tener en cuenta que la estructura de control solo esta disponible
     *   dentro de este archivo.
     *
	 *  @param caller		Puntero a la funcion donde fue llamado errorHook. Implementado solo a
	 *  					fines de trazabilidad de errores
	 *
	 *  @return none.
***************************************************************************************************/
void __attribute__((weak)) errorHook(void *caller)  {
	/*
	 * Revisar el contenido de control_OS.error para obtener informacion. Utilizar os_getError()
	 */
	while(1);
}



/*************************************************************************************************
	 *  @brief Tarea Idle (segundo plano)
     *
     *  @details
     *   Esta tarea se ejecuta solamente cuando todas las demas tareas estan en estado bloqueado.
     *   Puede ser redefinida por el usuario.
     *
	 *  @param none
	 *
	 *  @return none.
	 *
	 *  @warning		No debe utilizarse ninguna funcion API del OS dentro de esta funcion. No
	 *  				debe ser causa de un re-scheduling.
***************************************************************************************************/
void __attribute__((weak)) idleTask(void)  {
	while(1)  {
		__WFI();
	}
}





/*==================[definicion de funciones de OS]=================================*/


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
	 * y la tarea no se inicializa. La tarea idle debe ser exceptuada del conteo de cantidad maxima
	 * de tareas porque si al momento de iniciar el sistema ya se definieron la cantidad maxima de
	 * tareas posible, la tarea idle seria la numero 9 y la primer condicion es falsa.
	 */

	if(control_OS.cantidad_Tareas < MAX_TASK_COUNT || entryPoint == idleTask)  {

		task->stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;					//necesario para bit thumb
		task->stack[STACK_SIZE/4 - PC_REG] = (uint32_t)entryPoint;		//direccion de la tarea (ENTRY_POINT)
		task->stack[STACK_SIZE/4 - LR] = (uint32_t)returnHook;			//Retorno de la tarea (no deberia darse)

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
		 * la misma en estado READY. Todas las tareas se crean en estado READY.
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
		/*
		 * En el caso que se hayan excedido la cantidad de tareas que se pueden definir, se actualiza
		 * el ultimo error generado en la estructura de control del OS y se llama a errorHook y se
		 * envia informacion de quien es quien la invoca.
		 */
		control_OS.error = ERR_OS_CANT_TAREAS;
		errorHook(os_InitTarea);
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
	 * Es necesaria la inicializacion de la tarea idle, la cual no es visible al usuario
	 * El usuario puede eventualmente poblarla de codigo o redefinirla, pero no debe
	 * inicializarla ni definir una estructura para la misma.
	 */
	initTareaIdle();

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
	 * NOTA: La ultima tarea de este vector sera siempre la tarea idle
	 */

	for (uint8_t i = 0; i < MAX_TASK_COUNT_W_IDLE; i++)  {
		if(i>=control_OS.cantidad_Tareas)
			control_OS.listaTareas[i] = NULL;
	}
}




/*************************************************************************************************
	 *  @brief Extrae el codigo de error de la estructura de control del OS.
     *
     *  @details
     *   La estructura de control del OS no es visible al usuario, por lo que se facilita una API
     *   para extraer el ultimo codigo de error ocurrido, para su posterior tratamiento. Esta
     *   funcion puede ser utilizada dentro de errorHook
     *
	 *  @param 		None.
	 *  @return     Ultimo error ocurrido dentro del OS.
	 *  @see errorHook
***************************************************************************************************/
int32_t os_getError(void)  {
	return control_OS.error;
}


/*************************************************************************************************
	 *  @brief Inicializacion de la tarea idle.
     *
     *  @details
     *   Esta funcion es una version reducida de os_initTarea para la tarea idle. Como esta tarea
     *   debe estar siempre presente y el usuario no la inicializa, los argumentos desaparecen
     *   y se toman estructura y entryPoint fijos. Tampoco se contabiliza entre las tareas
     *   disponibles (no se actualiza el contador de cantidad de tareas). El id de esta tarea
     *   se establece como 255 (0xFF) para indicar que es una tarea especial.
     *
	 *  @param 		None.
	 *  @return     None
	 *  @see os_InitTarea
***************************************************************************************************/
static void initTareaIdle(void)  {
	tareaIdle.stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;					//necesario para bit thumb
	tareaIdle.stack[STACK_SIZE/4 - PC_REG] = (uint32_t)idleTask;		//direccion de la tarea (ENTRY_POINT)
	tareaIdle.stack[STACK_SIZE/4 - LR] = (uint32_t)returnHook;			//Retorno de la tarea (no deberia darse)


	tareaIdle.stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;
	tareaIdle.stack_pointer = (uint32_t) (tareaIdle.stack + STACK_SIZE/4 - FULL_STACKING_SIZE);


	tareaIdle.entry_point = idleTask;
	tareaIdle.id = 0xFF;
	tareaIdle.estado = TAREA_READY;
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
	bool salir = false;
	uint8_t cant_bloqueadas = 0;


	/*
	 * El scheduler recibe la informacion desde la variable estado de sistema si es el primer ingreso
	 * desde el ultimo reset. Si esto es asi, determina que la tarea actual es la primer tarea.
	 * Esto tiene aun validez porque todas las tareas se crean con estado READY.
	 * En esta version, se agrega una bandera por la cual el scheduler indica en la estructura de
	 * control del OS si es necesario hacer un cambio de contexto
	 *
	 */
	if (control_OS.estado_sistema == OS_FROM_RESET)  {
		control_OS.tarea_actual = (tarea*) control_OS.listaTareas[0];
		control_OS.cambioContextoNecesario = true;
	}
	else {
		/*
		 * En el caso que no sea el primer ingreso desde el reset, se comienza a
		 * iterar sobre el vector de tareas. La primer linea determina que el indice
		 * jamas sera mayor a cantidad_Tareas - 1. La primer tarea que se encuentre
		 * en estado ready sera la proxima tarea a ejecutar.
		 * Si la siguiente tarea en el vector tiene estado BLOCKED, se vuelve a iterar,
		 * incrementando el contador de tareas bloqueadas. Cuando la cantidad de tareas
		 * bloqueadas es igual a la cantidad de tareas presentes en el sistema, la tarea
		 * que se ejecuta es la tarea idle.
		 * Recordar que aunque todas las tareas definidas por el usuario esten bloqueadas
		 * la tarea Idle solamente puede tomar estados READY y RUNNING.
		 */

		indice = control_OS.tarea_actual->id;

		while(!salir)  {

			indice = (++indice) % control_OS.cantidad_Tareas;

			switch (((tarea*)control_OS.listaTareas[indice])->estado) {

			case TAREA_READY:
				control_OS.tarea_siguiente = (tarea*) control_OS.listaTareas[indice];
				control_OS.cambioContextoNecesario = true;
				salir = true;
				break;

			case TAREA_BLOCKED:
				cant_bloqueadas++;
				if (cant_bloqueadas == control_OS.cantidad_Tareas)  {
					control_OS.tarea_siguiente = &tareaIdle;
					control_OS.cambioContextoNecesario = true;
					salir = true;
				}
				break;

			/*
			 * El unico caso que la siguiente tarea este en estado RUNNING es que
			 * todas las demas tareas excepto la tarea corriendo actualmente esten en
			 * estado BLOCKED, con lo que un cambio de contexto no es necesario, porque
			 * se sigue ejecutando la misma tarea
			 */
			case TAREA_RUNNING:
				control_OS.cambioContextoNecesario = false;
				salir = true;
				break;

			/*
			 * En el caso que lleguemos al caso default, la tarea tomo un estado
			 * el cual es invalido, por lo que directamente se llama errorHook
			 * y se actualiza la variable de ultimo error
			 */
			default:
				control_OS.error = ERR_OS_SCHEDULING;
				errorHook(scheduler);
			}
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

	/*
	 * Se checkea la bandera correspondiente para verificar si es necesario un cambio de
	 * contexto. En caso afirmativo, se lanza PendSV
	 */

	if(control_OS.cambioContextoNecesario)
		setPendSV();


	/*
	 * Luego de determinar cual es la tarea siguiente segun el scheduler, se ejecuta la funcion
	 * tickhook.
	 */

	tickHook();
}



/*************************************************************************************************
	 *  @brief Setea la bandera correspondiente para lanzar PendSV.
     *
     *  @details
     *   Esta funcion simplemente es a efectos de simplificar la lectura del programa. Setea
     *   la bandera comrrespondiente para que se ejucute PendSV
     *
	 *  @param 		None
	 *  @return     None
***************************************************************************************************/
static void setPendSV(void)  {
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
	 * de la tarea corriendo actualmente. Ahora que el estado BLOCKED esta implementado, se debe hacer
	 * un assert de si la tarea actual fue expropiada mientras estaba corriendo o si la expropiacion
	 * fue hecha de manera prematura dado que paso a estado BLOCKED. En el segundo caso, solamente
	 * se puede pasar de BLOCKED a READY a partir de un evento.
	 * Se carga en la variable sp_siguiente el stack pointer de la tarea siguiente, que fue definida
	 * por el scheduler. Se actualiza la misma a estado RUNNING y se retorna al handler de PendSV
	 */
	else {
		control_OS.tarea_actual->stack_pointer = sp_actual;

		if (control_OS.tarea_actual->estado == TAREA_RUNNING)
			control_OS.tarea_actual->estado = TAREA_READY;

		sp_siguiente = control_OS.tarea_siguiente->stack_pointer;

		control_OS.tarea_actual = control_OS.tarea_siguiente;
		control_OS.tarea_actual->estado = TAREA_RUNNING;
	}

	/*
	 * Indicamos que luego de retornar de esta funcion, ya no es necesario un cambio de contexto
	 * porque se acaba de gestionar.
	 */
	control_OS.cambioContextoNecesario = false;

	return sp_siguiente;
}
