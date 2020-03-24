	/*
		.syntax unified

		Two slightly different syntaxes are support for ARM and THUMB instructions.
		The default, divided, uses the old style where ARM and THUMB instructions had their own,
		separate syntaxes. The new, unified syntax, which can be selected via the
		.syntax directive
	*/

	.syntax unified
	.global SysTick_Handler



	/*
		Se cambia a la seccion .data, donde se almacenan las variables en RAM
		Para ver data types en assembler
			--> http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0175m/Cbhifdhe.html
	*/
	.data
		i: .word 0		//Variable de muestra. Equivale a: uint16_t i = 0;

		.extern sp_durante
		.extern stackFrame


	/*
		Se cambia a la seccion .text, donde se almacena el programa en flash
	*/
	.text

	/*
		Indicamos que la proxima funcion debe ser tratada como codigo thumb al ser compilada
		Ver documentacion al respecto para mas detalles
	*/
	.thumb_func


#define sp_value		R4
#define sp_pointer		R5
#define contador		R6
#define aux				R7
#define stack_frame_dir	R8




SysTick_Handler:
	mrs sp_value,msp					//pasamos el valor del Stack pointer actual a R4
	ldr sp_pointer, =sp_durante			//puntero a la variable global. Equivale a R5 = &sp_durante;
	str sp_value, [sp_pointer]			// *sp_pointer = sp_value --> sp_durante = MSP

	ldr contador,=8						//cargamos el valor 8 al contador (son 8 registros, del SP+7 al SP)
	ldr stack_frame_dir, =stackFrame


loop:
	sub contador,1							//contador--;

	ldr aux,[sp_value,contador, LSL 2]		//cargamos aux con los valores uint32_t del stack en orden inverso
   											//aux = *(sp_pointer + (contador << 2))

	str aux,[stack_frame_dir,contador, LSL 2]   //*(stack_frame_dir + (contador << 2)) = aux


	cmp contador,0								//contador == 0 ?. SI: Ya tenemos los valores en el array global
	bne loop									//				   NO: branch loop

return:
	bx lr		//branch indirect. Al cargarse este valor en el PC, el core ejecuta auto-unstacking
