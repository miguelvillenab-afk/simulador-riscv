# Simulador RISC-V de Miguel

Proyecto de la Tarea 5 de Arquitectura de Computadores, desarrollado en C17.

## Etapa actual: memoria y decodificación

La memoria es byte-addressable, little-endian y dispersa en páginas de 4 KiB. El decodificador identifica las 37 instrucciones requeridas de RV32I, extrae sus registros y reconstruye los inmediatos de los formatos I, S, B, U y J. Las codificaciones fuera de alcance se marcan como inválidas.

## Compilar

```sh
cmake -S . -B build
cmake --build build
```

La ejecución de instrucciones y la terminal interactiva se incorporarán en las siguientes etapas.
