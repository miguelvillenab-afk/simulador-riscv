# Simulador RISC-V de Miguel

Proyecto de la Tarea 5 de Arquitectura de Computadores, desarrollado en C17.

## Etapa actual: memoria

La memoria es byte-addressable, usa orden little-endian y reserva páginas de 4 KiB solo cuando se escriben. Incluye lecturas y escrituras de 8, 16 y 32 bits, así como carga de bloques de bytes.

## Compilar

Se necesita CMake 3.16 o posterior y un compilador de C17.

```sh
cmake -S . -B build
cmake --build build
```

Las siguientes etapas añadirán decodificación de instrucciones, ejecución de CPU y controles interactivos de terminal.
