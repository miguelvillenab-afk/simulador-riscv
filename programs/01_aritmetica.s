# Suma 7 + 5, guarda el resultado en memoria y se detiene en un bucle propio.
# Los .word son instrucciones RV32I codificadas directamente; no son pseudoinstrucciones.
.text
.globl _start
_start:
    .word 0x00700293  # addi x5, x0, 7
    .word 0x00500313  # addi x6, x0, 5
    .word 0x006283B3  # add  x7, x5, x6
    .word 0x10702023  # sw   x7, 0x100(x0)
    .word 0x0000006F  # jal  x0, 0
