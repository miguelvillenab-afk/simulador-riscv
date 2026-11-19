#ifndef RISCV_DECODER_H
#define RISCV_DECODER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_INVALID = 0,
    OP_LB, OP_LH, OP_LW, OP_LBU, OP_LHU,
    OP_ADDI, OP_SLLI, OP_SLTI, OP_SLTIU, OP_XORI, OP_SRLI, OP_SRAI, OP_ORI, OP_ANDI,
    OP_AUIPC, OP_LUI,
    OP_SB, OP_SH, OP_SW,
    OP_ADD, OP_SUB, OP_SLL, OP_SLT, OP_SLTU, OP_XOR, OP_SRL, OP_SRA, OP_OR, OP_AND,
    OP_BEQ, OP_BNE, OP_BLT, OP_BGE, OP_BLTU, OP_BGEU,
    OP_JALR, OP_JAL
} Op;

typedef struct {
    Op op;
    uint32_t raw;
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint32_t immediate; /* Inmediato extendido como patrón de 32 bits. */
} Instruction;

bool decode_instruction(uint32_t raw, Instruction *instruction);
const char *op_name(Op op);
void format_instruction(const Instruction *instruction, char *buffer, size_t capacity);

#endif
