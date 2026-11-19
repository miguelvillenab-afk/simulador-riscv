# Guarda 0x80, lo carga con y sin signo y guarda ambas versiones en memoria.
.text
.globl _start
_start:
    .word 0x08000293  # addi x5, x0, 128
    .word 0x20500023  # sb   x5, 0x200(x0)
    .word 0x20000303  # lb   x6, 0x200(x0)    -> 0xFFFFFF80
    .word 0x20000383  # lbu  x7, 0x200(x0)    -> 0x00000080
    .word 0x20602223  # sw   x6, 0x204(x0)
    .word 0x20702423  # sw   x7, 0x208(x0)
    .word 0x0000006F  # jal  x0, 0
