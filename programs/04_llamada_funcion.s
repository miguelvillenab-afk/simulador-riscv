# Una función incrementa a0 en 7 y retorna con jalr.
.text
.globl _start
_start:
    .word 0x00500513  # addi a0, x0, 5
    .word 0x00C000EF  # jal  ra, +12           ; llamada a function
    .word 0x10A02423  # sw   a0, 0x108(x0)     ; guarda 12
    .word 0x00C0006F  # jal  x0, +12           ; saltar al final
function:
    .word 0x00750513  # addi a0, a0, 7
    .word 0x00008067  # jalr x0, 0(ra)         ; retornar
done:
    .word 0x0000006F  # jal  x0, 0
