#include "cli.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMMAND_LIMIT 8
#define RUN_LIMIT 10000000u
#define MEMORY_DISPLAY_LIMIT 256u

static const char *const abi_names[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3",
    "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static bool parse_number(const char *text, uint32_t *value) {
    if (text == NULL || *text == '\0' || *text == '-' || *text == '+') return false;
    errno = 0;
    char *end = NULL;
    int base = (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) ? 16 : 10;
    unsigned long long parsed = strtoull(text, &end, base);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) return false;
    *value = (uint32_t)parsed;
    return true;
}

static bool parse_register(const char *text, uint32_t *index) {
    if (text == NULL) return false;
    if (text[0] == 'x' && text[1] != '\0') {
        uint32_t n;
        if (parse_number(text + 1, &n) && n < 32) { *index = n; return true; }
    }
    if (strcmp(text, "fp") == 0) { *index = 8; return true; }
    for (uint32_t i = 0; i < 32; ++i)
        if (strcmp(text, abi_names[i]) == 0) { *index = i; return true; }
    return false;
}

static size_t split_words(char *line, char **words, size_t limit) {
    size_t count = 0;
    char *cursor = line;
    while (*cursor != '\0') {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') ++cursor;
        if (*cursor == '\0') break;
        if (count == limit) return limit + 1;
        words[count++] = cursor;
        while (*cursor != '\0' && *cursor != ' ' && *cursor != '\t' && *cursor != '\r' && *cursor != '\n') ++cursor;
        if (*cursor != '\0') *cursor++ = '\0';
    }
    return count;
}

static void show_pc(const Cpu *cpu) { printf("pc = 0x%08" PRIX32 "\n", cpu->pc); }

static void show_registers(const Cpu *cpu, char **names, size_t count) {
    if (count == 0) {
        for (uint32_t i = 0; i < 32; ++i)
            printf("x%-2" PRIu32 " %-4s = 0x%08" PRIX32 "\n", i, abi_names[i], cpu->registers[i]);
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        uint32_t reg;
        if (!parse_register(names[i], &reg)) {
            printf("Registro desconocido: '%s'. Usa x0-x31 o nombres como a0, sp, ra.\n", names[i]);
            return;
        }
    }
    for (size_t i = 0; i < count; ++i) {
        uint32_t reg;
        (void)parse_register(names[i], &reg);
        printf("%s = 0x%08" PRIX32 "\n", names[i], cpu->registers[reg]);
    }
}

static void show_memory(const Cpu *cpu, const char *start_text, const char *count_text) {
    uint32_t start, count = 1;
    if (!parse_number(start_text, &start) || (count_text != NULL && !parse_number(count_text, &count)) ||
        count == 0 || count > MEMORY_DISPLAY_LIMIT) {
        printf("Uso: mem <direccion> [cantidad 1-%u], en decimal o hexadecimal (0x...).\n", MEMORY_DISPLAY_LIMIT);
        return;
    }
    printf("Memoria desde 0x%08" PRIX32 " (%" PRIu32 " bytes):\n", start, count);
    for (uint32_t i = 0; i < count; ++i) {
        if (i % 16 == 0) printf("0x%08" PRIX32 ":", start + i);
        printf(" %02" PRIX8, memory_read8(&cpu->memory, start + i));
        if (i % 16 == 15 || i + 1 == count) putchar('\n');
    }
}

static void show_step(const StepResult *step) {
    char assembly[96];
    format_instruction(&step->instruction, assembly, sizeof(assembly));
    printf("PC 0x%08" PRIX32 " | 0x%08" PRIX32 " | %s\n",
           step->pc_before, step->raw, assembly);
    if (step->wrote_register)
        printf("  Registro: x%u cambia de 0x%08" PRIX32 " a 0x%08" PRIX32 "\n",
               step->register_index, step->register_before, step->register_after);
    if (step->wrote_memory) {
        printf("  Memoria: se escriben %u byte(s) desde 0x%08" PRIX32 " con 0x%0*" PRIX32 "\n",
               step->memory_width, step->memory_address, (int)(step->memory_width * 2),
               step->memory_value & (step->memory_width == 1 ? 0xffu :
                                     step->memory_width == 2 ? 0xffffu : 0xffffffffu));
    }
    if (step->instruction.op >= OP_BEQ && step->instruction.op <= OP_BGEU)
        printf("  Rama condicional: %s\n", step->branch_taken ? "tomada" : "no tomada");
    printf("  PC siguiente: 0x%08" PRIX32 "\n", step->pc_after);
}

static void show_help(void) {
    puts("Comandos disponibles:");
    puts("  step [n]             Ejecuta n instrucciones y explica sus cambios.");
    puts("  run                  Ejecuta hasta un bucle final, un error o el límite.");
    puts("  pc                   Muestra el contador de programa.");
    puts("  regs [registro ...]  Muestra todos o registros específicos (x5, a0, sp...).");
    puts("  mem <direccion> [n]  Muestra n bytes desde una dirección.");
    puts("  reset                Recarga el programa y reinicia el estado.");
    puts("  help                 Muestra esta ayuda.");
    puts("  exit                 Cierra el simulador.");
}

static bool execute_command(Cpu *cpu, const char *program_path, char *line) {
    char *words[COMMAND_LIMIT];
    size_t count = split_words(line, words, COMMAND_LIMIT);
    if (count == 0) return true;
    if (count > COMMAND_LIMIT) { puts("Demasiados argumentos."); return true; }

    if (strcmp(words[0], "exit") == 0 || strcmp(words[0], "quit") == 0) return false;
    if (strcmp(words[0], "help") == 0) { show_help(); return true; }
    if (strcmp(words[0], "pc") == 0 && count == 1) { show_pc(cpu); return true; }
    if (strcmp(words[0], "regs") == 0) { show_registers(cpu, words + 1, count - 1); return true; }
    if (strcmp(words[0], "mem") == 0 && (count == 2 || count == 3)) {
        show_memory(cpu, words[1], count == 3 ? words[2] : NULL); return true;
    }
    if (strcmp(words[0], "reset") == 0 && count == 1) {
        size_t loaded = 0;
        if (cpu_load_program(cpu, program_path, &loaded))
            printf("Programa recargado: %zu bytes desde 0x00000000.\n", loaded);
        else printf("Error: %s.\n", cpu->error);
        return true;
    }
    if (strcmp(words[0], "step") == 0) {
        uint32_t amount = 1;
        if (count > 2 || (count == 2 && (!parse_number(words[1], &amount) || amount == 0))) {
            puts("Uso: step [cantidad positiva]."); return true;
        }
        for (uint32_t i = 0; i < amount; ++i) {
            StepResult result;
            if (!cpu_step(cpu, &result)) { printf("Error: %s. El estado queda en el PC actual.\n", cpu->error); break; }
            show_step(&result);
        }
        return true;
    }
    if (strcmp(words[0], "run") == 0 && count == 1) {
        uint64_t executed = 0;
        while (executed < RUN_LIMIT) {
            uint32_t before = cpu->pc;
            StepResult result;
            if (!cpu_step(cpu, &result)) {
                printf("Ejecución detenida después de %" PRIu64 " instrucciones. Error: %s.\n", executed, cpu->error);
                return true;
            }
            ++executed;
            if (cpu->pc == before) {
                printf("Ejecución detenida después de %" PRIu64 " instrucciones: bucle propio en 0x%08" PRIX32 ".\n",
                       executed, before);
                return true;
            }
        }
        printf("Ejecución pausada tras %" PRIu64 " instrucciones (límite por sesión). Puedes usar run para continuar.\n", executed);
        return true;
    }
    if (strcmp(words[0], "pc") == 0 || strcmp(words[0], "mem") == 0) {
        puts("Uso incorrecto. Escribe help para ver los comandos."); return true;
    }
    printf("Comando desconocido o argumentos inválidos: '%s'. Escribe help.\n", words[0]);
    return true;
}

int cli_run(Cpu *cpu, const char *program_path) {
    char *line = NULL;
    size_t capacity = 0;
    while (true) {
        fputs("riscv> ", stdout);
        fflush(stdout);
        size_t length = 0;
        int ch;
        while ((ch = fgetc(stdin)) != EOF && ch != '\n') {
            if (length + 1 >= capacity) {
                size_t next_capacity = capacity == 0 ? 128 : capacity * 2;
                char *next = realloc(line, next_capacity);
                if (next == NULL) { free(line); fputs("No hay memoria para leer el comando.\n", stderr); return 1; }
                line = next;
                capacity = next_capacity;
            }
            line[length++] = (char)ch;
        }
        if (ch == EOF && length == 0) break;
        if (capacity == 0) {
            line = malloc(1);
            if (line == NULL) { fputs("No hay memoria para leer el comando.\n", stderr); return 1; }
            capacity = 1;
        }
        line[length] = '\0';
        if (!execute_command(cpu, program_path, line)) break;
    }
    free(line);
    puts("Simulador cerrado.");
    return 0;
}
