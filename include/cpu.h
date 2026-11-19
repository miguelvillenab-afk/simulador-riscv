#ifndef RISCV_CPU_H
#define RISCV_CPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "decoder.h"
#include "memory.h"

typedef struct {
    uint32_t pc_before;
    uint32_t pc_after;
    uint32_t raw;
    Instruction instruction;
    bool wrote_register;
    uint8_t register_index;
    uint32_t register_before;
    uint32_t register_after;
    bool wrote_memory;
    uint32_t memory_address;
    uint8_t memory_width;
    uint32_t memory_value;
    bool branch_taken;
} StepResult;

typedef struct {
    uint32_t pc;
    uint32_t registers[32];
    Memory memory;
    uint64_t steps;
    char error[160];
} Cpu;

void cpu_init(Cpu *cpu);
void cpu_destroy(Cpu *cpu);
void cpu_reset(Cpu *cpu);
bool cpu_load_program(Cpu *cpu, const char *path, size_t *loaded_bytes);
bool cpu_step(Cpu *cpu, StepResult *result);

#endif
