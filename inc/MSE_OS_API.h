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



/********************************************************************************
 * Definicion de la estructura para las colas
 *******************************************************************************/
struct _cola  {
	uint8_t data[QUEUE_HEAP_SIZE];
	tarea* tarea_asociada;
	uint16_t indice_head;
	uint16_t indice_tail;
	uint16_t size_elemento;
};

typedef struct _cola osCola;


void os_Delay(uint32_t ticks);

void os_SemaforoInit(osSemaforo* sem);
void os_SemaforoTake(osSemaforo* sem);
void os_SemaforoGive(osSemaforo* sem);

void os_ColaInit(osCola* cola, uint16_t datasize);
void os_ColaWrite(osCola* cola, void* dato);
void os_ColaRead(osCola* cola, void* dato);


#endif /* ISO_I_2020_MSE_OS_INC_MSE_OS_API_H_ */
