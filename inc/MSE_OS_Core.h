/*
 * MSE_OS_Core.h
 *
 *  Created on: 26 mar. 2020
 *      Author: gonza
 */

#ifndef ISO_I_2020_MSE_OS_INC_MSE_OS_CORE_H_
#define ISO_I_2020_MSE_OS_INC_MSE_OS_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"


/************************************************************************************
 * 			Tamaño del stack predefinido para cada tarea expresado en bytes
 ***********************************************************************************/

#define STACK_SIZE 256

//----------------------------------------------------------------------------------



/************************************************************************************
 * 	Posiciones dentro del stack de los registros que lo conforman
 ***********************************************************************************/

#define XPSR			1
#define PC_REG			2
#define LR				3
#define R12				4
#define R3				5
#define R2				6
#define R1				7
#define R0				8
#define LR_PREV_VALUE	9
#define R4				10
#define R5				11
#define R6				12
#define R7				13
#define R8				14
#define R9				15
#define R10 			16
#define R11 			17

//----------------------------------------------------------------------------------


/************************************************************************************
 * 			Valores necesarios para registros del stack frame inicial
 ***********************************************************************************/

#define INIT_XPSR 	1 << 24				//xPSR.T = 1
#define EXEC_RETURN	0xFFFFFFF9			//retornar a modo thread con MSP, FPU no utilizada

//----------------------------------------------------------------------------------


/************************************************************************************
 * 						Definiciones varias
 ***********************************************************************************/
#define STACK_FRAME_SIZE			8
#define FULL_STACKING_SIZE 			17	//16 core registers + valor previo de LR

#define TASK_NAME_SIZE				10	//tamaño array correspondiente al nombre
#define MAX_TASK_COUNT				8	//cantidad maxima de tareas para este OS

#define MAX_PRIORITY		0			//maxima prioridad que puede tener una tarea
#define MIN_PRIORITY		3			//minima prioridad que puede tener una tarea

#define PRIORITY_COUNT		(MIN_PRIORITY-MAX_PRIORITY)+1	//cantidad de prioridades asignables



/*==================[definicion codigos de error de OS]=================================*/
#define ERR_OS_CANT_TAREAS		-1
#define ERR_OS_SCHEDULING		-2



/*==================[definicion de datos para el OS]=================================*/

/********************************************************************************
 * Definicion de los estados posibles para las tareas
 *******************************************************************************/

enum _estadoTarea  {
	TAREA_READY,
	TAREA_RUNNING,
	TAREA_BLOCKED
};

typedef enum _estadoTarea estadoTarea;


/********************************************************************************
 * Definicion de los estados posibles de nuestro OS
 *******************************************************************************/

enum _estadoOS  {
	OS_NORMAL_RUN,
	OS_FROM_RESET
};

typedef enum _estadoOS estadoOS;


/********************************************************************************
 * Definicion de la estructura para cada tarea
 *******************************************************************************/
struct _tarea  {
	uint32_t stack[STACK_SIZE/4];
	uint32_t stack_pointer;
	void *entry_point;
	uint8_t id;
	estadoTarea estado;
	uint8_t prioridad;
};

typedef struct _tarea tarea;



/********************************************************************************
 * Definicion de la estructura de control para el sistema operativo
 *******************************************************************************/
struct _osControl  {
	void *listaTareas[MAX_TASK_COUNT];			//array de punteros a tareas
	int32_t error;								//variable que contiene el ultimo error generado
	uint8_t cantidad_Tareas;					//cantidad de tareas definidas por el usuario
	uint8_t cantTareas_prioridad[PRIORITY_COUNT];	//cada posicion contiene cuantas tareas tienen la misma prioridad

	estadoOS estado_sistema;					//Informacion sobre el estado del OS
	bool cambioContextoNecesario;				//Esta bandera indica si el scheduler determino un cambio de contexto

	tarea *tarea_actual;				//definicion de puntero para tarea actual
	tarea *tarea_siguiente;			//definicion de puntero para tarea siguiente
};
typedef struct _osControl osControl;


/*==================[definicion de prototipos]=================================*/

void os_InitTarea(void *entryPoint, tarea *task, uint8_t prioridad);
void os_Init(void);
int32_t os_getError(void);





#endif /* ISO_I_2020_MSE_OS_INC_MSE_OS_CORE_H_ */
