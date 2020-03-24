# MSE_OS
Sistema Operativo desarrollado para la materia "Implementacion de sistemas operativos I"

Este sistema operativo tiene fines solamente didacticos, no es recomendable para utilizarlo en produccion de dispositivos y o productos.

## CLASE 1
La clase 1 esta avocada a que el estudiante logre comprender el mecanismo de atencion de interrupciones y la formación del stack frame, que luego sera ampliamente utilizada para escribir el OS propietario de cada uno

El código fuente muestra un ejemplo para ver detenidamente el proceso del stack frame al atender una interrupción cualquiera (en este caso SysTick).

Se dispone de variables globales para observar los valores del MSP antes, durante y despues de la interrupcion, y un array que almacena el stack frame estando dentro de la interrupcion, para ser evaluado fuera de ella.
