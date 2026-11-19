# Suma los enteros del 1 al 5. Muestra addi, add, bge y jal hacia atrás.
.text
.globl _start
_start:
    .word 0x00100293  # addi x5, x0, 1         ; contador
    .word 0x00500313  # addi x6, x0, 5         ; límite
    .word 0x00000393  # addi x7, x0, 0         ; suma
loop:
    .word 0x005383B3  # add  x7, x7, x5
    .word 0x0062D663  # bge  x5, x6, +12       ; salir después de sumar 5
    .word 0x00128293  # addi x5, x5, 1
    .word 0xFF5FF06F  # jal  x0, -12           ; volver a loop
done:
    .word 0x10702223  # sw   x7, 0x104(x0)     ; resultado: 15
    .word 0x0000006F  # jal  x0, 0
