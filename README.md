# Mi simulador de RISC-V

Este proyecto es mi implementación de la Tarea 5 de Arquitectura de Computadores. Estoy construyendo un simulador de la ISA base RISC-V de 32 bits (RV32I) en C17.

La idea es poder cargar un programa ya compilado y ver, paso a paso, qué hace la CPU. Cuando ejecuto `step`, el simulador muestra la instrucción, el valor de sus bits y los cambios que produce en los registros, la memoria y el contador de programa. Así puedo seguir la ejecución sin tener que deducir cada cambio mirando solamente el código máquina.

## Compilar

Necesitas CMake 3.16 o posterior y un compilador compatible con C17. Desde la carpeta del proyecto, ejecuta:

```sh
cmake -S . -B build
cmake --build build
```

## Abrir un programa

El simulador carga un archivo binario crudo en la dirección `0x00000000`:

```sh
./build/riscv-miguel programa.bin
```

El archivo debe contener palabras de máquina RISC-V, no un ejecutable ELF de Linux. Si usas CPUlator para compilar un programa, exporta el rango de memoria en formato **Raw**.

## Comandos

| Comando | Para qué sirve |
| --- | --- |
| `step` | Avanza una instrucción y explica qué cambió. |
| `step 5` | Avanza cinco instrucciones, mostrando cada paso. |
| `run` | Ejecuta hasta encontrar un bucle propio, un error o el límite de seguridad. |
| `pc` | Consulta la dirección de la siguiente instrucción. |
| `regs` | Muestra los 32 registros. |
| `regs a0 sp x5` | Muestra solo los registros indicados. Se aceptan alias como `a0`, `sp` y `ra`. |
| `mem 0x100 4` | Muestra cuatro bytes desde la dirección `0x100`. |
| `reset` | Vuelve a cargar el programa y reinicia la CPU. |
| `help` | Muestra la lista de comandos. |
| `exit` | Sale del simulador. |

Al ejecutar un `step`, la salida tiene esta forma:

```text
PC 0x00000000 | 0x00700293 | addi x5, x0, 7
  Registro: x5 cambia de 0x00000000 a 0x00000007
  PC siguiente: 0x00000004
```

## Qué incluye esta versión

La CPU mantiene el PC, los 32 registros y una memoria direccionable por byte en formato little-endian. La memoria usa páginas dispersas de 4 KiB: solo reserva una página cuando el programa escribe en ella. El registro `x0` siempre conserva el valor cero.

El decodificador y la CPU cubren las 37 instrucciones pedidas en el anexo A del enunciado. Una instrucción inválida o un PC desalineado producen un mensaje y dejan la ejecución detenida para poder inspeccionar el estado.

Por ahora el simulador no carga ELF ni implementa `ecall`, GDB o una interfaz gráfica. Esas funciones aparecen como ideas adicionales en el enunciado; la versión actual se concentra en ejecutar e inspeccionar los programas RV32I requeridos.

## Programas de ejemplo

Preparé cuatro ejemplos pequeños para que cada uno muestre una parte distinta del simulador:

| Archivo | Qué quiero mostrar | Resultado que busca el programa |
| --- | --- | --- |
| `programs/01_aritmetica.bin` | Suma y escritura de una palabra. | `x7 = 12`; el valor 12 queda en `0x100`. |
| `programs/02_memoria_signo.bin` | Cargar el mismo byte con y sin extensión de signo. | `x6 = 0xFFFFFF80` con `lb` y `x7 = 0x80` con `lbu`. |
| `programs/03_suma_bucle.bin` | Un bucle con rama y salto hacia atrás. | La suma del 1 al 5 es 15 y queda en `0x104`. |
| `programs/04_llamada_funcion.bin` | Llamar una función con `jal` y regresar con `jalr`. | `a0 = 12`; el valor queda guardado en `0x108`. |

Las fuentes `.s` muestran cada palabra de máquina con un comentario que explica la instrucción. Para recrear los binarios crudos:

```sh
python3 programs/export_raw.py
```

Ese script solo empaqueta las palabras explícitas en orden little-endian; no es un ensamblador general.

## Organización del código

- `src/memory.c`: lecturas, escrituras y carga de bytes en memoria.
- `src/decoder.c`: identificación de instrucciones y reconstrucción de inmediatos.
- `src/cpu.c`: estado arquitectural y ejecución de una instrucción.
- `src/cli.c`: comandos de terminal y explicación de cada paso.
- `programs/`: fuentes y binarios crudos de demostración.
