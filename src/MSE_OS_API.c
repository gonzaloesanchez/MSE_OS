/*
 * MSE_OS_API.c
 *
 *  Created on: 12 abr. 2020
 *      Author: gonza
 */


#include "MSE_OS_API.h"


/*************************************************************************************************
	 *  @brief delay no preciso en base a ticks del sistema
     *
     *  @details
     *   Para utilizar un delay en el OS se vale del tick de sistema para contabilizar cuantos
     *   ticks debe una tarea estar bloqueada.
     *
	 *  @param		ticks	Cantidad de ticks de sistema que esta tarea debe estar bloqueada
	 *  @return     None.
	 *  @warning	No puede llamarse a delay desde un handler, produce un error de OS
***************************************************************************************************/
void os_Delay(uint32_t ticks)  {
	tarea* tarea_actual;

	/*
	 * Esta prohibido llamar esta funcion de API desde un handler, genera un error que
	 * detiene el OS
	 */
	if(os_getEstadoSistema() == OS_IRQ_RUN)  {
		os_setError(ERR_OS_DELAY_FROM_ISR,os_Delay);
	}

	/*
	 * En esta version se modifica la funcion delay en algunos aspectos:
	 * 1) Ya no es necesario corroborar que la funcion este en RUNNING al momento de cargar los
	 * ticks dado que la obtencion del puntero a la estructura de la tarea y la asignacion
	 * de ticks se hace dentro de una seccion critica
	 * 2) Nada del codigo se ejecuta si la variable ticks vale 0
	 */

	if(ticks > 0)  {
		os_enter_critical();

		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		tarea_actual = os_getTareaActual();
		tarea_actual->ticks_bloqueada = ticks;
		//----------------------------------------------------------------------------------------------------

		os_exit_critical();

		/*
		 * El proximo bloque while tiene la finalidad de asegurarse que la tarea solo se desbloquee
		 * en el momento que termine la cuenta de ticks. Si por alguna razon la tarea se vuelve a
		 * ejecutar antes que termine el periodo de bloqueado, queda atrapada.
		 * La bandera delay activo es puesta en false por el SysTick. En teoria esto no deberia volver
		 * a ejecutarse dado que el scheduler no vuelve a darle CPU hasta que no pase a estado READY
		 *
		 */

		while (tarea_actual->ticks_bloqueada > 0)  {
			tarea_actual->estado = TAREA_BLOCKED;
			os_CpuYield();
		}
	}
}


/*************************************************************************************************
	 *  @brief Inicializacion de un semaforo binario
     *
     *  @details
     *   Antes de utilizar cualquier semaforo binario en el sistema, debe inicializarse el mismo.
     *   Todos los semaforos se inicializan tomados
     *
	 *  @param		sem		Semaforo a inicializar
	 *  @return     None.
***************************************************************************************************/
void os_SemaforoInit(osSemaforo* sem)  {
	sem->tomado = true;
	sem->tarea_asociada = NULL;
}



/*************************************************************************************************
	 *  @brief Tomar un semaforo
     *
     *  @details
     *   Esta funcion es utilizada para tomar un semaforo cualquiera.
     *
	 *  @param		sem		Semaforo a tomar
	 *  @return     None.
***************************************************************************************************/
void os_SemaforoTake(osSemaforo* sem)  {
	bool Salir = false;
	tarea* tarea_actual;

	/*
	 * En el caso de que otra tarea desbloquee por error la tarea que se acaba de
	 * bloquear con el semaforo (en el caso que este tomado) el bloque while se
	 * encarga de volver a bloquearla hasta tanto no se haga un give
	 */
	while (!Salir)  {

		/*
		 * Si el semaforo esta tomado, la tarea actual debe bloquearse y se
		 * mantiene un puntero a la estructura de la tarea actual, la que
		 * recibe el nombre de tarea asociada. Luego se hace un CPU yield
		 * dado que no se necesita mas el CPU hasta que se libere el semaforo.
		 *
		 * Si el semaforo estaba libre, solamente se marca como tomado y se
		 * retorna.
		 *
		 * Se agrega una seccion critica al momento de obtener la tarea actual e
		 * interactuar con su estado
		 */
		if(sem->tomado)  {
			os_enter_critical();

			//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			tarea_actual = os_getTareaActual();
			tarea_actual->estado = TAREA_BLOCKED;
			sem->tarea_asociada = tarea_actual;
			//---------------------------------------------------------------------------

			os_exit_critical();
			os_CpuYield();
		}
		else  {
			sem->tomado = true;
			Salir = true;
		}

	}
}



/********************************************************************************
	 *  @brief Liberar un semaforo
     *
     *  @details
     *   Esta funcion es utilizada para liberar un semaforo cualquiera.
     *
	 *  @param		sem		Semaforo a liberar
	 *  @return     None.
 *******************************************************************************/
void os_SemaforoGive(osSemaforo* sem)  {

	/*
	 * Por seguridad, se deben hacer dos checkeos antes de hacer un give sobre
	 * el semaforo. En el caso de que se ambas condiciones sean verdaderas, se
	 * libera y se actualiza la tarea correspondiente a estado ready.
	 */

	if (sem->tomado == true &&	sem->tarea_asociada != NULL)  {
		sem->tomado = false;
		sem->tarea_asociada->estado = TAREA_READY;

		/*
		 * Si es llamada desde una interrupcion, se debe indicar que es necesario efectuar
		 * un scheduling, porque seguramente existe una tarea esperando este evento
		 */
		if (os_getEstadoSistema() == OS_IRQ_RUN)
			 os_setScheduleDesdeISR(true);
	}
}


/*************************************************************************************************
	 *  @brief Inicializacion de una cola
     *
     *  @details
     *   Antes de utilizar cualquier cola en el sistema, debe inicializarse la misma.
     *   Todas las colas se inicializan vacias y sin una tarea asociada. Aqui se determina
     *   cuantos elementos (espacios) tendra disponible la cola dado el tamaño de la cola
     *   en bytes definida por la constante QUEUE_HEAP_SIZE y el tamaño en bytes de cada
     *   elemento que se desea almacenar. Es una forma facil de determinar los limites
     *   de los indices head y tail en el momento de la operacion. Se puede volver a inicializar
     *   una cola para resetearla y cambiar el tipo de datos que contiene
     *
	 *  @param		cola		Cola a inicializar
	 *  @param		datasize	Tamaño de los elementos que seran almacenados en la cola.
	 *  						Debe ser pasado mediante la funcion sizeof()
	 *  @return     None.
	 *
	 *  @warning	Esta inicializacion fija el tamaño de cada elemento, por lo que no vuelve
	 *  			a consultarse en otras funciones, pasar datos con otros tamaños en funciones
	 *  			de escritura y lectura puede dar lugar a corrupcion de datos.
***************************************************************************************************/
void os_ColaInit(osCola* cola, uint16_t datasize)  {
	cola->indice_head = 0;
	cola->indice_tail = 0;
	cola->tarea_asociada = NULL;
	cola->size_elemento = datasize;
}


/*************************************************************************************************
	 *  @brief Escritura en una cola
     *
     *  @details
     *
     *
	 *  @param		cola		Cola donde escribir el dato
	 *  @param		dato		Puntero a void del dato a escribir
	 *  @return     None.
***************************************************************************************************/
void os_ColaWrite(osCola* cola, void* dato)  {
	uint16_t index_h;					//variable para legibilidad
	uint16_t elementos_total;		//variable para legibilidad
	tarea* tarea_actual;

	index_h = cola->indice_head * cola->size_elemento;
	elementos_total = QUEUE_HEAP_SIZE / cola->size_elemento;


	/*
	 * el primer bloque determina tres cosas, que gracias a la disposicion de los
	 * parentesis se dan en un orden especifico:
	 * 1) Se determina si head == tail, con lo que la cola estaria vacia
	 * 2) Sobre el resultado de 1) se determina si existe una tarea asociada
	 * 3) Si la cola esta vacia, y el puntero a la tarea asociada es valido, se
	 * 		verifica si la tarea asociada esta bloqueada
	 * Estas condiciones en ese orden determinan si se trato de leer de una cola vacia
	 * y la tarea que quizo leer se bloqueo porque la misma estaba vacia. Como
	 * seguramente en este punto se escribe un dato a la misma, esa tarea tiene que
	 * pasar a ready
	 */

	if(((cola->indice_head == cola->indice_tail) && cola->tarea_asociada != NULL) &&
		cola->tarea_asociada->estado == TAREA_BLOCKED)  {

		cola->tarea_asociada->estado = TAREA_READY;

		/*
		 * Si es llamada desde una interrupcion, se debe indicar que es necesario efectuar
		 * un scheduling, porque seguramente existe una tarea esperando este evento
		 */
		if (os_getEstadoSistema() == OS_IRQ_RUN)
			os_setScheduleDesdeISR(true);

	}



	 /*
	 * En el caso de que se quiera escribir una cola desde un ISR y este
	 * llena, la operacion es abortada (no se puede bloquear un handler)
	 */
	if (os_getEstadoSistema() == OS_IRQ_RUN &&
			(cola->indice_head + 1) % elementos_total == cola->indice_tail)  {

		os_setWarning(WARN_OS_QUEUE_FULL_ISR);
		return;		//operacion abortada
	}


	/*
	 * El siguiente bloque while determina que hasta que la cola no tenga lugar
	 * disponible, no se avance. Si no tiene lugar se bloquea la tarea actual
	 * que es la que esta tratando de escribir y luego se hace un yield
	 */
	while((cola->indice_head + 1) % elementos_total == cola->indice_tail)  {

		os_enter_critical();

		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		tarea_actual = os_getTareaActual();
		tarea_actual->estado = TAREA_BLOCKED;
		cola->tarea_asociada = tarea_actual;
		//---------------------------------------------------------------------------

		os_exit_critical();
		os_CpuYield();
	}

	/*
	 * Si la cola tiene lugar, se escribe mediante la funcion memcpy que copia un
	 * bloque completo de memoria iniciando desde la direccion apuntada por el
	 * primer elemento. Como data es un vector del tipo uint8_t, la aritmetica
	 * de punteros es byte a byte (consecutivos) y se logra el efecto deseado
	 * Esto permite guardar datos definidos por el usuario, como ser estructuras
	 * de datos completas. Luego se actualiza el undice head y se limpia la tarea
	 * asociada, dado que ese puntero ya no tiene utilidad
	 */

	memcpy(cola->data+index_h,dato,cola->size_elemento);
	cola->indice_head = (cola->indice_head + 1) % elementos_total;
	cola->tarea_asociada = NULL;
}

void os_ColaRead(osCola* cola, void* dato)  {
	uint16_t elementos_total;		//variable para legibilidad
	uint16_t index_t;					//variable para legibilidad
	tarea* tarea_actual;


	index_t = cola->indice_tail * cola->size_elemento;
	elementos_total = QUEUE_HEAP_SIZE / cola->size_elemento;


	/*
	 * el primer bloque determina tres cosas, que gracias a la disposicion de los
	 * parentesis se dan en un orden especifico:
	 * 1) Se determina si la cola esta llena (head+1)%CANT_ELEMENTOS == tail
	 * 2) Sobre el resultado de 1) se determina si existe una tarea asociada
	 * 3) Si la cola esta llena, y el puntero a la tarea asociada es valido, se
	 * 		verifica si la tarea asociada esta bloqueada
	 * Estas condiciones en ese orden determinan si se trato de escribir en una cola
	 * llena y la tarea que quizo escribir se bloqueo porque la misma estaba llena. Como
	 * seguramente en este punto se lee un dato de la misma, esa tarea tiene que
	 * pasar a ready
	 */

	if((( (cola->indice_head + 1) % elementos_total == cola->indice_tail) &&
			cola->tarea_asociada != NULL) &&
			cola->tarea_asociada->estado == TAREA_BLOCKED)  {

		cola->tarea_asociada->estado = TAREA_READY;

		/*
		 * Si es llamada desde una interrupcion, se debe indicar que es necesario efectuar
		 * un scheduling, porque seguramente existe una tarea esperando este evento
		 */
		if (os_getEstadoSistema() == OS_IRQ_RUN)
			os_setScheduleDesdeISR(true);
	}


	/*
	 * En el caso de que se quiera leer una cola desde un ISR y este
	 * vacia, la operacion es abortada (no se puede bloquear un handler)
	 */
	if (os_getEstadoSistema() == OS_IRQ_RUN && cola->indice_head == cola->indice_tail)  {
		os_setWarning(WARN_OS_QUEUE_EMPTY_ISR);
		return;		//operacion abortada
	}

	/*
	 * El siguiente bloque while determina que hasta que la cola no tenga un dato
	 * disponible, no se avance. Si no hay un dato que leer, se bloquea la tarea
	 * actual que es la que esta tratando de leer un dato y luego se hace un yield
	 */

	while(cola->indice_head == cola->indice_tail)  {
		os_enter_critical();

		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		tarea_actual = os_getTareaActual();
		tarea_actual->estado = TAREA_BLOCKED;
		cola->tarea_asociada = tarea_actual;
		//---------------------------------------------------------------------------

		os_exit_critical();
		os_CpuYield();
	}

	/*
	 * Si la cola tiene datos, se lee mediante la funcion memcpy que copia un
	 * bloque completo de memoria iniciando desde la direccion apuntada por el
	 * primer elemento. Como data es un vector del tipo uint8_t, la aritmetica
	 * de punteros es byte a byte (consecutivos) y se logra el efecto deseado
	 * Esto permite guardar datos definidos por el usuario, como ser estructuras
	 * de datos completas. Luego se actualiza el undice head y se limpia la tarea
	 * asociada, dado que ese puntero ya no tiene utilidad
	 */

	memcpy(dato,cola->data+index_t,cola->size_elemento);
	cola->indice_tail = (cola->indice_tail + 1) % elementos_total;
	cola->tarea_asociada = NULL;

}

