# Simulador RISC-V de Miguel

Simulador educativo de la ISA base RISC-V de 32 bits (RV32I), escrito desde cero en C. La primera versión usa una interfaz de terminal para cargar un binario crudo, ejecutar instrucciones bajo control del usuario y consultar el estado de la CPU.

## Idea propia

El comando `step` explica cada instrucción: muestra el PC, el word de máquina, una representación legible de la operación y los cambios en registros, memoria y PC. La meta es que la ejecución sea fácil de seguir mientras se estudia cómo se transforma el estado arquitectural.

## Compilar

Se necesita CMake 3.16 o posterior y un compilador de C que admita C17.

```sh
cmake -S . -B build
cmake --build build
```

## Ejecutar

El simulador recibe un archivo binario crudo como argumento. El programa se carga desde `0x00000000`.

```sh
./build/riscv-miguel programa.bin
```

Comandos disponibles:

| Comando | Efecto |
| --- | --- |
| `step [n]` | Ejecuta hasta `n` instrucciones y explica los cambios de cada una. |
| `run` | Ejecuta de corrido hasta un bucle propio, un error o el límite de seguridad. |
| `pc` | Muestra el contador de programa. |
| `regs [xN ...]` | Muestra todos los registros o los seleccionados por número o alias ABI. |
| `mem <dirección> [cantidad]` | Muestra bytes de memoria desde la dirección indicada. |
| `reset` | Recarga el binario y reinicia la CPU. |
| `help` | Muestra la ayuda. |
| `exit` | Cierra el simulador. |

## Alcance inicial

La memoria es byte-addressable, little-endian y dispersa: las páginas de 4 KiB se reservan cuando se escriben. El estado incluye PC, 32 registros (x0 permanece en cero) y memoria de direcciones de 32 bits. La implementación contempla las 37 instrucciones obligatorias del anexo A. La carga de ELF, `ecall`, GDB y una interfaz gráfica no forman parte del alcance inicial.

Los archivos de programa deben ser binarios crudos exportados desde CPUlator con el rango del programa. No se acepta el ELF de Linux como si fuera ese formato.

## Programas de demostración

En `programs/` hay cuatro ejemplos propios. Sus fuentes muestran explícitamente cada palabra de máquina; el script de exportación solo empaqueta esos words en binarios little-endian y no es un ensamblador general.

```sh
python3 programs/export_raw.py
./build/riscv-miguel programs/01_aritmetica.bin
```

| Programa | Qué demuestra | Resultado esperado |
| --- | --- | --- |
| `01_aritmetica.bin` | `addi`, `add` y `sw` | `x7 = 12`; palabra en `0x100` vale 12. |
| `02_memoria_signo.bin` | `sb`, `lb`, `lbu` y `sw` | `x6 = 0xFFFFFF80`; `x7 = 0x80`. |
| `03_suma_bucle.bin` | Bucle, rama `bge` y salto hacia atrás | `x7 = 15`; resultado en `0x104`. |
| `04_llamada_funcion.bin` | `jal`, `jalr` y retorno | `a0 = 12`; resultado en `0x108`. |
