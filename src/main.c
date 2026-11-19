#include "cli.h"
#include "cpu.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <programa.bin>\n", argv[0]);
        return 2;
    }
    Cpu cpu;
    cpu_init(&cpu);
    size_t loaded = 0;
    if (!cpu_load_program(&cpu, argv[1], &loaded)) {
        fprintf(stderr, "Error: %s.\n", cpu.error);
        cpu_destroy(&cpu);
        return 1;
    }
    printf("Programa '%s' cargado en 0x00000000 (%zu bytes).\n", argv[1], loaded);
    puts("Escribe help para ver los comandos.");
    int result = cli_run(&cpu, argv[1]);
    cpu_destroy(&cpu);
    return result;
}
