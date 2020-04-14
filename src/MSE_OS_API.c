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
***************************************************************************************************/
void os_Delay(uint32_t ticks)  {
	tarea* tarea_actual;

	/*
	 * La estructura control_OS solo puede ser accedida desde el archivo core, por lo que
	 * se provee una funcion para obtener la tarea actual (equivale a acceder a
	 * control_OS.tarea_actual)
	 */
	tarea_actual = os_getTareaActual();

	/*
	 * Se carga la cantidad de ticks a la tarea actual si la misma esta en running
	 * y si los ticks son mayores a cero
	 */
	if (tarea_actual->estado == TAREA_RUNNING && ticks > 0)  {

		tarea_actual->ticks_bloqueada = ticks;

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
	 * La estructura control_OS solo puede ser accedida desde el archivo core, por lo que
	 * se provee una funcion para obtener la tarea actual (equivale a acceder a
	 * control_OS.tarea_actual)
	 */
	tarea_actual = os_getTareaActual();

	if (tarea_actual->estado == TAREA_RUNNING)  {

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
			 * retorna
			 */
			if(sem->tomado)  {
				tarea_actual->estado = TAREA_BLOCKED;
				sem->tarea_asociada = tarea_actual;
				os_CpuYield();
			}
			else  {
				sem->tomado = true;
				Salir = true;
			}
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
	tarea* tarea_actual;

	tarea_actual = os_getTareaActual();

	/*
	 * Por seguridad, se deben hacer varios checkeos antes de hacer un give sobre
	 * el semaforo. En el caso de que se den todas las condiciones, se libera y se
	 * actualiza la tarea correspondiente a estado ready.
	 */

	if (tarea_actual->estado == TAREA_RUNNING &&
			sem->tomado == true &&
			sem->tarea_asociada != NULL)  {

		sem->tomado = false;
		sem->tarea_asociada->estado = TAREA_READY;
	}
}

