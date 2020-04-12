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
		 *
		 */

		while (tarea_actual->ticks_bloqueada > 0)  {
			tarea_actual->estado = TAREA_BLOCKED;
			os_CpuYield();
		}
	}
}
