/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"

#include "MSE_OS_Core.h"


/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Global data declaration]==============================*/

tarea g_sTarea1, g_sTarea2,g_sTarea3;
tarea g_sTarea4, g_sTarea5,g_sTarea6;
tarea g_sTarea7, g_sTarea8,g_sTarea9;

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
	uint32_t h = 0;
	uint32_t i = 0;
	while (1) {
		h++;
		i++;
	}
}

void tarea2(void)  {
	uint32_t j = 0;
	uint32_t k = 0;

	while (1) {
		j++;
		k++;
	}
}

void tarea3(void)  {
	uint32_t l = 0;
	uint32_t m = 0;

	while (1) {
		l++;
		m++;
	}
}



/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTarea(tarea1, &g_sTarea1);
	os_InitTarea(tarea2, &g_sTarea2);
	os_InitTarea(tarea3, &g_sTarea3);


	os_Init();

	while (1) {
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/
