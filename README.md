# Simulador RISC-V de Miguel

Proyecto de la Tarea 5 de Arquitectura de Computadores, desarrollado en C17.

## Etapa actual: estado y ejecución de CPU

La CPU mantiene el PC, 32 registros (x0 se conserva en cero) y memoria. `cpu_step` obtiene la instrucción, la decodifica, ejecuta su operación y actualiza el estado. El núcleo implementa las 37 instrucciones obligatorias de RV32I y reporta errores de instrucción inválida y PC desalineado.

La memoria es byte-addressable, little-endian y dispersa en páginas de 4 KiB. El decodificador reconstruye los inmediatos de los formatos I, S, B, U y J.

## Compilar

```sh
cmake -S . -B build
cmake --build build
```

La terminal interactiva y los ejemplos de programas se añadirán en siguientes etapas.
