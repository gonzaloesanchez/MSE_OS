/*==================[inclusions]=============================================*/

#include <stdlib.h>
#include "main.h"
#include "board.h"
#include "MSE_OS_Core.h"
#include "MSE_OS_API.h"
#include "sapi.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

#define PRIORIDAD_0		0
#define PRIORIDAD_1		1
#define PRIORIDAD_3		3

/*==================[Global data declaration]==============================*/

tarea g_sTarea1, g_sTarea2, g_sTarea3;	//prioridad 0
tarea g_sBotones;	//prioridad 3

osSemaforo colaTarea1, colaTarea2;

struct _mydata  {
	float dato_float;
	int32_t dato_int;
	char letras[3];
};

typedef struct _mydata my_data;

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms
}


/*==================[Definicion de tareas para el OS]==========================*/
void tarea1(void)  {
	uint32_t i;
	my_data datos;

	while (1) {

		os_ColaRead(&colaTarea1,&datos);
		gpioWrite(LED1,true);
		os_Delay(1000);
		gpioWrite(LED1,false);
		memset(&datos,0x00,sizeof(my_data));

	}
}

void tarea2(void)  {
	uint32_t j;
	my_data datos;

	while (1) {

		os_ColaRead(&colaTarea2,&datos);
		gpioWrite(LED2,true);
		os_Delay(1000);
		gpioWrite(LED2,false);
		memset(&datos,0x00,sizeof(my_data));
	}
}

void botones(void)  {
	my_data datos_enviar;

	/*
	 * CUIDADO: asi como esta esto, en 1[s] se envian 10 datos a la cola
	 */
	while(1)  {
		if(!gpioRead( TEC1 ))  {
			datos_enviar.dato_float = 3.1415;
			datos_enviar.dato_int = -1;
			datos_enviar.letras[0] = 'A';
			datos_enviar.letras[1] = 'B';
			datos_enviar.letras[2] = 'C';
			os_ColaWrite(&colaTarea1,&datos_enviar);
		}

		if(!gpioRead( TEC2 ))  {
			datos_enviar.dato_float = 2.7182;
			datos_enviar.dato_int = -1000;
			datos_enviar.letras[0] = 'E';
			datos_enviar.letras[1] = 'F';
			datos_enviar.letras[2] = 'G';
			os_ColaWrite(&colaTarea2,&datos_enviar);
		}

		os_Delay(100);
	}
}


/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTarea(tarea1, &g_sTarea1,PRIORIDAD_0);
	os_InitTarea(tarea2, &g_sTarea2,PRIORIDAD_0);
	os_InitTarea(botones, &g_sBotones,PRIORIDAD_3);

	os_ColaInit(&colaTarea1,sizeof(my_data));
	os_ColaInit(&colaTarea2,sizeof(my_data));


	os_Init();

	while (1) {
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
