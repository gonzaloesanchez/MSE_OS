/*
 * MSE_OS_API.h
 *
 *  Created on: 12 abr. 2020
 *      Author: gonza
 */

#ifndef ISO_I_2020_MSE_OS_INC_MSE_OS_API_H_
#define ISO_I_2020_MSE_OS_INC_MSE_OS_API_H_


#include "MSE_OS_Core.h"


/********************************************************************************
 * Definicion de la estructura para los semaforos
 *******************************************************************************/
struct _semaforo  {
	tarea* tarea_asociada;
	bool tomado;
};

typedef struct _semaforo osSemaforo;




void os_Delay(uint32_t ticks);
void os_SemaforoInit(osSemaforo* sem);
void os_SemaforoTake(osSemaforo* sem);
void os_SemaforoGive(osSemaforo* sem);


#endif /* ISO_I_2020_MSE_OS_INC_MSE_OS_API_H_ */
