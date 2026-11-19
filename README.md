# Simulador RISC-V de Miguel

Proyecto de la Tarea 5 de Arquitectura de Computadores. Se desarrollará en C17 y empezará con una interfaz de terminal para cargar y ejecutar programas binarios RV32I.

## Compilar

Se necesita CMake 3.16 o posterior y un compilador de C17.

```sh
cmake -S . -B build
cmake --build build
```

El simulador se implementará por etapas: configuración del proyecto, memoria, decodificación, ejecución de instrucciones e interfaz interactiva.
