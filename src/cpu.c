#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>

static int64_t as_signed(uint32_t value) {
    return (value & 0x80000000u) ? (int64_t)value - 0x100000000LL : (int64_t)value;
}

static uint32_t arithmetic_right(uint32_t value, uint32_t amount) {
    if (amount == 0) return value;
    uint32_t shifted = value >> amount;
    return (value & 0x80000000u) ? shifted | (0xffffffffu << (32u - amount)) : shifted;
}

static void set_error(Cpu *cpu, const char *message) {
    snprintf(cpu->error, sizeof(cpu->error), "%s", message);
}

void cpu_init(Cpu *cpu) {
    cpu->pc = 0;
    for (size_t i = 0; i < 32; ++i) cpu->registers[i] = 0;
    memory_init(&cpu->memory);
    cpu->steps = 0;
    cpu->error[0] = '\0';
}

void cpu_destroy(Cpu *cpu) { memory_destroy(&cpu->memory); }

void cpu_reset(Cpu *cpu) {
    cpu->pc = 0;
    for (size_t i = 0; i < 32; ++i) cpu->registers[i] = 0;
    cpu->steps = 0;
    cpu->error[0] = '\0';
    memory_clear(&cpu->memory);
}

bool cpu_load_program(Cpu *cpu, const char *path, size_t *loaded_bytes) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(cpu->error, sizeof(cpu->error), "no se pudo abrir el archivo '%s'", path);
        return false;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file); set_error(cpu, "no se pudo leer el archivo"); return false;
    }
    long file_size = ftell(file);
    if (file_size <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file); set_error(cpu, file_size == 0 ? "el archivo está vacío" : "tamaño de archivo inválido");
        return false;
    }
    if ((uint64_t)file_size > 0x100000000ULL) {
        fclose(file); set_error(cpu, "el programa excede el espacio de direcciones de 32 bits"); return false;
    }
    uint8_t *bytes = malloc((size_t)file_size);
    if (bytes == NULL) {
        fclose(file); set_error(cpu, "no hay memoria para cargar el programa"); return false;
    }
    size_t read_count = fread(bytes, 1, (size_t)file_size, file);
    bool read_ok = read_count == (size_t)file_size && !ferror(file);
    fclose(file);
    if (!read_ok) {
        free(bytes); set_error(cpu, "no se pudo leer el archivo completo"); return false;
    }

    cpu_reset(cpu);
    bool loaded = memory_load(&cpu->memory, 0, bytes, read_count);
    free(bytes);
    if (!loaded) {
        cpu_reset(cpu); set_error(cpu, "no hay memoria para cargar el programa"); return false;
    }
    if (loaded_bytes != NULL) *loaded_bytes = read_count;
    return true;
}

bool cpu_step(Cpu *cpu, StepResult *result) {
    cpu->error[0] = '\0';
    if (cpu->pc & 3u) {
        snprintf(cpu->error, sizeof(cpu->error), "PC desalineado: 0x%08X", cpu->pc);
        return false;
    }

    uint32_t raw = memory_read32(&cpu->memory, cpu->pc);
    Instruction instruction;
    if (!decode_instruction(raw, &instruction)) {
        snprintf(cpu->error, sizeof(cpu->error), "instrucción inválida 0x%08X en PC 0x%08X", raw, cpu->pc);
        return false;
    }

    StepResult out = {0};
    out.pc_before = cpu->pc;
    out.raw = raw;
    out.instruction = instruction;
    uint32_t a = cpu->registers[instruction.rs1];
    uint32_t b = cpu->registers[instruction.rs2];
    uint32_t imm = instruction.immediate;
    uint32_t next_pc = cpu->pc + 4u;
    uint32_t value = 0;
    bool writes_register = false;

#define WRITE_RD(v) do { value = (uint32_t)(v); writes_register = true; } while (0)
#define STORE8(addr, v) do { if (!memory_write8(&cpu->memory, (addr), (uint8_t)(v))) goto memory_error; } while (0)
#define STORE16(addr, v) do { if (!memory_write16(&cpu->memory, (addr), (uint16_t)(v))) goto memory_error; } while (0)
#define STORE32(addr, v) do { if (!memory_write32(&cpu->memory, (addr), (uint32_t)(v))) goto memory_error; } while (0)

    switch (instruction.op) {
    case OP_LB: { uint8_t v = memory_read8(&cpu->memory, a + imm); WRITE_RD((v & 0x80u) ? (uint32_t)(0xffffff00u | v) : v); break; }
    case OP_LH: { uint16_t v = memory_read16(&cpu->memory, a + imm); WRITE_RD((v & 0x8000u) ? (uint32_t)(0xffff0000u | v) : v); break; }
    case OP_LW: WRITE_RD(memory_read32(&cpu->memory, a + imm)); break;
    case OP_LBU: WRITE_RD(memory_read8(&cpu->memory, a + imm)); break;
    case OP_LHU: WRITE_RD(memory_read16(&cpu->memory, a + imm)); break;
    case OP_ADDI: WRITE_RD(a + imm); break;
    case OP_SLLI: WRITE_RD(a << imm); break;
    case OP_SLTI: WRITE_RD(as_signed(a) < as_signed(imm)); break;
    case OP_SLTIU: WRITE_RD(a < imm); break;
    case OP_XORI: WRITE_RD(a ^ imm); break;
    case OP_SRLI: WRITE_RD(a >> imm); break;
    case OP_SRAI: WRITE_RD(arithmetic_right(a, imm)); break;
    case OP_ORI: WRITE_RD(a | imm); break;
    case OP_ANDI: WRITE_RD(a & imm); break;
    case OP_AUIPC: WRITE_RD(cpu->pc + imm); break;
    case OP_LUI: WRITE_RD(imm); break;
    case OP_SB: out.wrote_memory = true; out.memory_address = a + imm; out.memory_width = 1; out.memory_value = b; STORE8(a + imm, b); break;
    case OP_SH: out.wrote_memory = true; out.memory_address = a + imm; out.memory_width = 2; out.memory_value = b; STORE16(a + imm, b); break;
    case OP_SW: out.wrote_memory = true; out.memory_address = a + imm; out.memory_width = 4; out.memory_value = b; STORE32(a + imm, b); break;
    case OP_ADD: WRITE_RD(a + b); break;
    case OP_SUB: WRITE_RD(a - b); break;
    case OP_SLL: WRITE_RD(a << (b & 31u)); break;
    case OP_SLT: WRITE_RD(as_signed(a) < as_signed(b)); break;
    case OP_SLTU: WRITE_RD(a < b); break;
    case OP_XOR: WRITE_RD(a ^ b); break;
    case OP_SRL: WRITE_RD(a >> (b & 31u)); break;
    case OP_SRA: WRITE_RD(arithmetic_right(a, b & 31u)); break;
    case OP_OR: WRITE_RD(a | b); break;
    case OP_AND: WRITE_RD(a & b); break;
    case OP_BEQ: out.branch_taken = a == b; break;
    case OP_BNE: out.branch_taken = a != b; break;
    case OP_BLT: out.branch_taken = as_signed(a) < as_signed(b); break;
    case OP_BGE: out.branch_taken = as_signed(a) >= as_signed(b); break;
    case OP_BLTU: out.branch_taken = a < b; break;
    case OP_BGEU: out.branch_taken = a >= b; break;
    case OP_JALR: next_pc = (a + imm) & ~1u; WRITE_RD(cpu->pc + 4u); break;
    case OP_JAL: next_pc = cpu->pc + imm; WRITE_RD(cpu->pc + 4u); break;
    case OP_INVALID: goto invalid_error;
    }

    if (instruction.op >= OP_BEQ && instruction.op <= OP_BGEU && out.branch_taken)
        next_pc = cpu->pc + imm;

    if (writes_register && instruction.rd != 0) {
        out.wrote_register = true;
        out.register_index = instruction.rd;
        out.register_before = cpu->registers[instruction.rd];
        out.register_after = value;
        cpu->registers[instruction.rd] = value;
    }
    cpu->registers[0] = 0;
    cpu->pc = next_pc;
    ++cpu->steps;
    out.pc_after = cpu->pc;
    *result = out;
    return true;

memory_error:
    set_error(cpu, "no hay memoria disponible para completar la escritura");
    return false;
invalid_error:
    set_error(cpu, "instrucción inválida");
    return false;
}
