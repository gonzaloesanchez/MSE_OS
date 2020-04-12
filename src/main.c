/*==================[inclusions]=============================================*/

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

tarea g_sTarea1;	//prioridad 0
tarea g_sTarea2;	//prioridad 1

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

	while (1) {
		gpioToggle(LED1);
		//os_Delay(1000);
	}
}

void tarea2(void)  {

	while (1) {
		gpioToggle(LEDB);
		os_Delay(200);
	}
}


/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTarea(tarea1, &g_sTarea1,PRIORIDAD_0);
	os_InitTarea(tarea2, &g_sTarea2,PRIORIDAD_0);
	os_Init();

	while (1) {
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
