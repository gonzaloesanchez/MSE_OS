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
static void ordenarPrioridades(void);
static int32_t partition(tarea** arr, int32_t l, int32_t h);


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
void os_InitTarea(void *entryPoint, tarea *task, uint8_t prioridad)  {
	static uint8_t id = 0;				//el id sera correlativo a medida que se generen mas tareas

	/*
	 * Al principio se efectua un pequeño checkeo para determinar si llegamos a la cantidad maxima de
	 * tareas que pueden definirse para este OS. En el caso de que se traten de inicializar mas tareas
	 * que el numero maximo soportado, se guarda un codigo de error en la estructura de control del OS
	 * y la tarea no se inicializa. La tarea idle debe ser exceptuada del conteo de cantidad maxima
	 * de tareas
	 */

	if(control_OS.cantidad_Tareas < MAX_TASK_COUNT)  {

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
		 * Se asigna la prioridad de la misma.
		 */
		task->entry_point = entryPoint;
		task->id = id;
		task->estado = TAREA_READY;
		task->prioridad = prioridad;

		/*
		 * Actualizacion de la estructura de control del OS, guardando el puntero a la estructura de tarea
		 * que se acaba de inicializar, y se actualiza la cantidad de tareas definidas en el sistema.
		 * Luego se incrementa el contador de id, dado que se le otorga un id correlativo a cada tarea
		 * inicializada, segun el orden en que se inicializan.
		 */
		control_OS.listaTareas[id] = task;
		control_OS.cantidad_Tareas++;
		control_OS.cantTareas_prioridad[prioridad]++;

		id++;
	}

	else {
		/*
		 * En el caso que se hayan excedido la cantidad de tareas que se pueden definir, se actualiza
		 * el ultimo error generado en la estructura de control del OS y se llama a errorHook y se
		 * envia informacion de quien es quien la invoca.
		 */
		os_setError(ERR_OS_CANT_TAREAS,os_InitTarea);
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
	 */

	for (uint8_t i = 0; i < MAX_TASK_COUNT; i++)  {
		if(i>=control_OS.cantidad_Tareas)
			control_OS.listaTareas[i] = NULL;
	}

	ordenarPrioridades();
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
     *   La prioridad tambien es 255, esta prioridad no existe, pero segun esta implementacion
     *   tampoco se utiliza
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
	tareaIdle.prioridad = 0xFF;
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
	static uint8_t indicePrioridad[PRIORITY_COUNT];		//indice de tareas a ejecutar segun prioridad
	uint8_t indiceArrayTareas = 0;
	uint8_t prioridad_actual = MAX_PRIORITY;			//Maxima prioridad al iniciar
	uint8_t cantBloqueadas_prioActual = 0;
	bool salir = false;
	uint8_t cant_bloqueadas = 0;
	estadoOS estado_anterior;


	/*
	 * El scheduler recibe la informacion desde la variable estado_sistema si es el primer ingreso
	 * desde el ultimo reset. Si esto es asi, se carga como tarea actual la tarea idle. Esto es
	 * mas prolijo que tener una bandera para cada situacion.
	 * Es necesario que el vector que lleva los indices de las tareas este a cero antes de comenzar
	 * lo que se logra con la funcion memset()
	 */
	if (control_OS.estado_sistema == OS_FROM_RESET)  {
		control_OS.tarea_actual = (tarea*) &tareaIdle;
		memset(indicePrioridad,0,sizeof(uint8_t) * PRIORITY_COUNT);
		control_OS.estado_sistema = OS_NORMAL_RUN;
	}

	/*
	 * Se comienza a iterar sobre el vector de tareas, pero teniendo en cuenta las
	 * prioridades de las tareas.
	 * En esta implementacion, durante la ejecucion de os_Init() se aplica la
	 * tecnica de quicksort sobre el vector que contiene la lista de tareas
	 * y se ordenan los punteros a tareas segun la prioridad que tengan. Los
	 * punteros a tareas quedan ordenados de mayor a menor prioridad.
	 * Gracias a eso, dividiendo el vector de punteros en cuantas prioridades haya
	 * podemos recorrer estas subsecciones manteniendo indices para cada prioridad
	 *
	 * La mecanica de RoundRobin para tareas de igual prioridad se mantiene,
	 * asimismo la determinacion de cuando la tarea idle debe ser ejecutada. Cuando
	 * se recorren todas las tareas de una prioridad y todas estan bloqueadas, entonces
	 * se pasa a la prioridad menor siguiente
	 *
	 * Recordar que aunque todas las tareas definidas por el usuario esten bloqueadas
	 * la tarea Idle solamente puede tomar estados READY y RUNNING.
	 */


	/*
	 * Puede darse el caso en que se haya invocado la funcion os_CpuYield() la cual hace una
	 * llamada al scheduler. Si durante la ejecucion del scheduler (la cual fue forzada) y
	 * esta siendo atendida en modo Thread ocurre una excepcion dada por el SysTick, habra una
	 * instancia del scheduler pendiente en modo trhead y otra corriendo en modo Handler invocada
	 * por el SysTick. Para evitar un doble scheduling, se controla que el sistema no este haciendo
	 * uno ya. En caso afirmativo se vuelve prematuramente
	 */
	if (control_OS.estado_sistema == OS_SCHEDULING)  {
		return;
	}

	/*
	 * Cambia el estado del sistema para que no se produzcan schedulings anidados cuando
	 * existen forzados por alguna API del sistema.
	 */
	control_OS.estado_sistema = OS_SCHEDULING;

	/*
	 * Como la determinacion de cuantas funciones estan en READY o BLOCKED es dinamica,
	 * se hace un bucle con un testigo para salir de el
	 */
	while(!salir)  {

		/*
		 * La variable indiceArrayTareas contiene la posicion real dentro del array listaTareas
		 * de la tarea siguiente a ser ejecutada. Se inicializa en 0
		 */
		indiceArrayTareas = 0;

		/*
		 * Puede darse el caso de que se hayan definido tareas de prioridades no contiguas, por
		 * ejemplo dos tareas de maxima prioridad y una de minima prioridad. Quiere decir que no
		 * existen tareas con prioridades entre estas dos. Este bloque if determina si existen
		 * funciones para la prioridad actual. Si no existen, se pasa a la prioridad menor
		 * siguiente
		 */
		if(control_OS.cantTareas_prioridad[prioridad_actual] > 0)  {

			/*
			 * esta linea asegura que el indice siempre este dentro de los limites de la subseccion
			 * que forman los punteros a las tareas de prioridad actual
			 */
			indicePrioridad[prioridad_actual] %= control_OS.cantTareas_prioridad[prioridad_actual];

			/*
			 * El bucle for hace una conversion de indices de subsecciones al indice real que debe
			 * usarse sobre el vector de punteros a tareas. Cuando se baja de prioridad debe sumarse
			 * el total de tareas que existen en la subseccion anterior. Recordar que el vector de
			 * tareas esta ordenado de mayor a menor
			 */
			for (int i=0; i<prioridad_actual;i++) {
				indiceArrayTareas += control_OS.cantTareas_prioridad[i];
			}
			indiceArrayTareas += indicePrioridad[prioridad_actual];




			switch (((tarea*)control_OS.listaTareas[indiceArrayTareas])->estado) {

			case TAREA_READY:
				control_OS.tarea_siguiente = (tarea*) control_OS.listaTareas[indiceArrayTareas];
				control_OS.cambioContextoNecesario = true;
				indicePrioridad[prioridad_actual]++;
				salir = true;
				break;

				/*
				 * Para el caso de las tareas bloqueadas, la variable cantBloqueadas_prioActual se utiliza
				 * para hacer el seguimiento de si se debe bajar un escalon de prioridad
				 * El bucle del scheduler se ejecuta completo, pasando por todas las prioridades cada vez
				 * hasta que encuentra una tarea en READY.
				 * La determinacion de la necesidad de ejecucion de la tarea idle sigue siendo la misma.
				 */
			case TAREA_BLOCKED:
				cant_bloqueadas++;
				cantBloqueadas_prioActual++;
				indicePrioridad[prioridad_actual]++;
				if (cant_bloqueadas == control_OS.cantidad_Tareas)  {
					control_OS.tarea_siguiente = &tareaIdle;
					control_OS.cambioContextoNecesario = true;
					salir = true;
				}
				else {
					if(cantBloqueadas_prioActual == control_OS.cantTareas_prioridad[prioridad_actual])  {
						cantBloqueadas_prioActual = 0;
						indicePrioridad[prioridad_actual] = 0;
						prioridad_actual++;
					}
				}
				break;

				/*
				 * El unico caso que la siguiente tarea este en estado RUNNING es que
				 * todas las demas tareas excepto la tarea corriendo actualmente esten en
				 * estado BLOCKED, con lo que un cambio de contexto no es necesario, porque
				 * se sigue ejecutando la misma tarea
				 */
			case TAREA_RUNNING:
				indicePrioridad[prioridad_actual]++;
				control_OS.cambioContextoNecesario = false;
				salir = true;
				break;

				/*
				 * En el caso que lleguemos al caso default, la tarea tomo un estado
				 * el cual es invalido, por lo que directamente se levanta un error de
				 * sistema actualizando la variable de ultimo error
				 */
			default:
				os_setError(ERR_OS_SCHEDULING,scheduler);
			}
		}
		else {
			indicePrioridad[prioridad_actual] = 0;
			prioridad_actual++;
		}
	}

	/*
	 * Antes de salir del scheduler se devuelve el sistema a su estado normal
	 */
	control_OS.estado_sistema = OS_NORMAL_RUN;

	/*
	 * Se checkea la bandera correspondiente para verificar si es necesario un cambio de
	 * contexto. En caso afirmativo, se lanza PendSV
	 */

	if(control_OS.cambioContextoNecesario)
		setPendSV();
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
	uint8_t i;
	tarea* task;		//variable para legibilidad

	/*
	 * Systick se encarga de actualizar todos los temporizadores por lo que se recorren
	 * todas las tareas que esten definidas y si tienen un valor de ticks de bloqueo mayor
	 * a cero, se decrementan en una unidad. Si este contador llega a cero, entonces
	 * se debe pasar la tarea a READY. Es conveniente hacerlo aqui dado que la condicion
	 * de que pase a descontar el ultimo tick se da en esta porcion de codigo
	 */
	i = 0;

	while (control_OS.listaTareas[i] != NULL)  {
		task = (tarea*)control_OS.listaTareas[i];

		if( task->ticks_bloqueada > 0 )  {
			if((--task->ticks_bloqueada == 0) && (task->estado == TAREA_BLOCKED))  {
				task->estado = TAREA_READY;
			}
		}

		i++;
	}


	/*
	 * Dentro del SysTick handler se llama al scheduler. Separar el scheduler de
	 * getContextoSiguiente da libertad para cambiar la politica de scheduling en cualquier
	 * estadio de desarrollo del OS. Recordar que scheduler() debe ser lo mas corto posible
	 */

	scheduler();


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

	/*
	 * Se indica en la estructura del OS que el cambio de contexto se esta por invocar
	 * Se hace antes de setear PendSV para no interferir con las barreras de datos
	 * y memoria
	 */
	control_OS.cambioContextoNecesario = false;

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
	 * Esta funcion efectua el cambio de contexto. Se guarda el MSP (sp_actual) en la variable
	 * correspondiente de la estructura de la tarea corriendo actualmente. Ahora que el estado
	 * BLOCKED esta implementado, se debe hacer un assert de si la tarea actual fue expropiada
	 * mientras estaba corriendo o si la expropiacion fue hecha de manera prematura dado que
	 * paso a estado BLOCKED. En el segundo caso, solamente se puede pasar de BLOCKED a READY
	 * a partir de un evento. Se carga en la variable sp_siguiente el stack pointer de la
	 * tarea siguiente, que fue definida por el scheduler. Se actualiza la misma a estado RUNNING
	 * y se retorna al handler de PendSV
	 */

	control_OS.tarea_actual->stack_pointer = sp_actual;

	if (control_OS.tarea_actual->estado == TAREA_RUNNING)
		control_OS.tarea_actual->estado = TAREA_READY;

	sp_siguiente = control_OS.tarea_siguiente->stack_pointer;

	control_OS.tarea_actual = control_OS.tarea_siguiente;
	control_OS.tarea_actual->estado = TAREA_RUNNING;


	/*
	 * Indicamos que luego de retornar de esta funcion, ya no es necesario un cambio de contexto
	 * porque se acaba de gestionar.
	 */
	control_OS.estado_sistema = OS_NORMAL_RUN;

	return sp_siguiente;
}



/*************************************************************************************************
	 *  @brief Fuerza una ejecucion del scheduler.
     *
     *  @details
     *   En los casos que un delay de una tarea comience a ejecutarse instantes luego de que
     *   ocurriese un scheduling, se despericia mucho tiempo hasta el proximo tick de sistema,
     *   por lo que se fuerza un scheduling y un cambio de contexto si es necesario.
     *
	 *  @param 		None
	 *  @return     None.
***************************************************************************************************/
void os_CpuYield(void)  {
	scheduler();
}



/*************************************************************************************************
	 *  @brief Devuelve una copia del puntero a estructura tarea actual.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una copia de la estructura de la tarea actual
     *
	 *  @param 		None
	 *  @return     puntero a la estructura de la tarea actual.
***************************************************************************************************/
tarea* os_getTareaActual(void)  {
	return control_OS.tarea_actual;
}



/*************************************************************************************************
	 *  @brief Devuelve una copia del valor del estado de sistema.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una copia del estado de sistema actual
     *
	 *  @param 		None
	 *  @return     estado del OS.
***************************************************************************************************/
estadoOS os_getEstadoSistema(void)  {
	return control_OS.estado_sistema;
}



/*************************************************************************************************
	 *  @brief Cambia el estado de sistema al pasado como argumento por esta funcion.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una manera de actualizar el estado del sistema en otros archivos
     *
	 *  @param 		None
	 *  @return     estado del OS.
***************************************************************************************************/
void os_setEstadoSistema(estadoOS estado)  {
	control_OS.estado_sistema = estado;
}


/*************************************************************************************************
	 *  @brief Setea la bandera de scheduling desde ISR.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una forma de setear la bandera correspondiente a la necesidad
     *   de ejecutar un scheduling antes de salir de una interrupcion
     *
	 *  @param 		value	El valor a escribir en la bandera correspondiente
	 *  @return     none.
***************************************************************************************************/
void os_setScheduleDesdeISR(bool value)  {
	control_OS.schedulingFromIRQ = value;
}


/*************************************************************************************************
	 *  @brief Devuelve una copia del valor del estado de sistema.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una forma de obtener el valor de la bandera correspondiente a la
     *   necesidad de ejecutar un scheduling antes de salir de una interrupcion
     *
	 *  @param 		None
	 *  @return     Valor de la bandera correspondiente.
***************************************************************************************************/
bool os_getScheduleDesdeISR(void)  {
	return control_OS.schedulingFromIRQ;
}


/*************************************************************************************************
	 *  @brief Levanta un error de sistema.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una forma de setear el codigo de error y lanza una ejecucion
     *   de errorHook
     *
	 *  @param 		err		El codigo de error que ha surgido
	 *  @param		caller	Puntero a la funcion que llama a esta funcion
	 *  @return     none.
***************************************************************************************************/
void os_setError(int32_t err, void* caller)  {
	control_OS.error = err;
	errorHook(caller);
}


/*************************************************************************************************
	 *  @brief Levanta un error de sistema.
     *
     *  @details
     *   En aras de mantener la estructura de control aislada solo en el archivo de core esta
     *   funcion proporciona una forma de setear el codigo de warning. Se almacena en la misma
     *   variable que error porque en OS sigue funcionando. En el caso de error, el OS deja de
     *   funcionar, con lo que son dos casos mutuamente excluyentes.
     *
	 *  @param 		warn	El codigo de advertencia que ha surgido
	 *  @return     none.
***************************************************************************************************/
void os_setWarning(int32_t warn)  {
	control_OS.error = warn;
}




/*************************************************************************************************
	 *  @brief Marca el inicio de una seccion como seccion critica.
     *
     *  @details
     *   Las secciones criticas son aquellas que deben ejecutar operaciones atomicas, es decir que
     *   no pueden ser interrumpidas. Con llamar a esta funcion, se otorga soporte en el OS
     *   para marcar un bloque de codigo como atomico
     *
	 *  @param 		None
	 *  @return     None
	 *  @see 		os_exit_critical
***************************************************************************************************/
inline void os_enter_critical()  {
	__disable_irq();
	control_OS.contador_critico++;
}


/*************************************************************************************************
	 *  @brief Marca el final de una seccion como seccion critica.
     *
     *  @details
     *   Las secciones criticas son aquellas que deben ejecutar operaciones atomicas, es decir que
     *   no pueden ser interrumpidas. Con llamar a esta funcion, se otorga soporte en el OS
     *   para marcar un bloque de codigo como atomico
     *
	 *  @param 		None
	 *  @return     None
	 *  @see 		os_enter_critical
***************************************************************************************************/
inline void os_exit_critical()  {
	if (--control_OS.contador_critico <= 0)  {
		control_OS.contador_critico = 0;
		__enable_irq();
	}
}



/*************************************************************************************************
	 *  @brief Ordena tareas de mayor a menor prioridad.
     *
     *  @details
     *   Ordena los punteros a las estructuras del tipo tarea que estan almacenados en la variable
     *   de control de OS en el array listadoTareas por prioridad, de mayor a menor. Para esto
     *   utiliza un algoritmo de quicksort. Esto da la posibilidad de cambiar la prioridad
     *   de cualquier tarea en tiempo de ejecucion.
     *
	 *  @param 		None
	 *  @return     None.
***************************************************************************************************/
static void ordenarPrioridades(void)  {
	// Create an auxiliary stack
	int32_t stack[MAX_TASK_COUNT];

	// initialize top of stack
	int32_t top = -1;
	int32_t l = 0;
	int32_t h = control_OS.cantidad_Tareas - 1;

	// push initial values of l and h to stack (indices a estructuras de tareas)
	stack[++top] = l;
	stack[++top] = h;

	// Keep popping from stack while is not empty
	while (top >= 0) {
		// Pop h and l
		h = stack[top--];
		l = stack[top--];

		// Set pivot element at its correct position
		// in sorted array
		int32_t p = partition(control_OS.listaTareas, l, h);

		// If there are elements on left side of pivot,
		// then push left side to stack
		if (p - 1 > l) {
			stack[++top] = l;
			stack[++top] = p - 1;
		}

		// If there are elements on right side of pivot,
		// then push right side to stack
		if (p + 1 < h) {
			stack[++top] = p + 1;
			stack[++top] = h;
		}
	}
}


/*************************************************************************************************
	 *  @brief Ordena tareas de mayor a menor prioridad.
     *
     *  @details
     *   Funcion de soporte para ordenarPrioridades. No debe llamarse fuera de mencionada
     *   funcion.
     *
	 *  @param 	arr		Puntero a la lista de punteros de estructuras de tareas a ordenar
	 *  @param	l		Inicio del vector a ordenar (puede ser un subvector)
	 *  @param	h		Fin del vector a ordenar (puede ser un subvector)
	 *  @return     	Retorna la posicion del pivot necesario para el algoritmo
***************************************************************************************************/
static int32_t partition(tarea** arr, int32_t l, int32_t h)  {
	tarea* x = arr[h];
	tarea* aux;
	int32_t i = (l - 1);

	for (int j = l; j <= h - 1; j++) {
		if (arr[j]->prioridad <= x->prioridad) {
			i++;
			//swap(&arr[i], &arr[j]);
			aux = arr[i];
			arr[i] = arr[j];
			arr[j] = aux;
		}
	}
	//swap(&arr[i + 1], &arr[h]);
	aux = arr[i+1];
	arr[i+1] = arr[h];
	arr[h] = aux;

	return (i + 1);
}



