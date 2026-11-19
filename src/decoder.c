#include "decoder.h"

#include <stdio.h>

static uint32_t field(uint32_t word, unsigned high, unsigned low) {
    return (word >> low) & ((1u << (high - low + 1u)) - 1u);
}

static uint32_t sign_extend(uint32_t value, unsigned width) {
    uint32_t mask = (1u << width) - 1u;
    uint32_t sign = 1u << (width - 1u);
    value &= mask;
    return (value ^ sign) - sign;
}

bool decode_instruction(uint32_t raw, Instruction *instruction) {
    instruction->op = OP_INVALID;
    instruction->raw = raw;
    instruction->rd = (uint8_t)field(raw, 11, 7);
    instruction->rs1 = (uint8_t)field(raw, 19, 15);
    instruction->rs2 = (uint8_t)field(raw, 24, 20);
    instruction->immediate = 0;

    uint32_t opcode = field(raw, 6, 0);
    uint32_t funct3 = field(raw, 14, 12);
    uint32_t funct7 = field(raw, 31, 25);

    switch (opcode) {
    case 0x03: { /* Loads */
        static const Op operations[8] = {OP_LB, OP_LH, OP_LW, OP_INVALID,
                                         OP_LBU, OP_LHU, OP_INVALID, OP_INVALID};
        instruction->op = operations[funct3];
        instruction->immediate = sign_extend(field(raw, 31, 20), 12);
        break;
    }
    case 0x13: { /* Immediate arithmetic and logic */
        static const Op operations[8] = {OP_ADDI, OP_SLLI, OP_SLTI, OP_SLTIU,
                                         OP_XORI, OP_SRLI, OP_ORI, OP_ANDI};
        instruction->op = operations[funct3];
        instruction->immediate = sign_extend(field(raw, 31, 20), 12);
        if (funct3 == 1 || funct3 == 5) {
            instruction->immediate = field(raw, 24, 20);
            if (funct3 == 1 && funct7 != 0) instruction->op = OP_INVALID;
            if (funct3 == 5) {
                if (funct7 == 0x20) instruction->op = OP_SRAI;
                else if (funct7 != 0) instruction->op = OP_INVALID;
            }
        }
        break;
    }
    case 0x17:
        instruction->op = OP_AUIPC;
        instruction->immediate = raw & 0xfffff000u;
        break;
    case 0x37:
        instruction->op = OP_LUI;
        instruction->immediate = raw & 0xfffff000u;
        break;
    case 0x23: { /* Stores */
        static const Op operations[8] = {OP_SB, OP_SH, OP_SW, OP_INVALID,
                                         OP_INVALID, OP_INVALID, OP_INVALID, OP_INVALID};
        uint32_t value = (field(raw, 31, 25) << 5) | field(raw, 11, 7);
        instruction->op = operations[funct3];
        instruction->immediate = sign_extend(value, 12);
        break;
    }
    case 0x33: /* Register arithmetic and logic */
        if (funct7 == 0) {
            static const Op operations[8] = {OP_ADD, OP_SLL, OP_SLT, OP_SLTU,
                                             OP_XOR, OP_SRL, OP_OR, OP_AND};
            instruction->op = operations[funct3];
        } else if (funct7 == 0x20 && funct3 == 0) instruction->op = OP_SUB;
        else if (funct7 == 0x20 && funct3 == 5) instruction->op = OP_SRA;
        break;
    case 0x63: { /* Conditional branches */
        static const Op operations[8] = {OP_BEQ, OP_BNE, OP_INVALID, OP_INVALID,
                                         OP_BLT, OP_BGE, OP_BLTU, OP_BGEU};
        uint32_t value = (field(raw, 31, 31) << 12) | (field(raw, 7, 7) << 11) |
                         (field(raw, 30, 25) << 5) | (field(raw, 11, 8) << 1);
        instruction->op = operations[funct3];
        instruction->immediate = sign_extend(value, 13);
        break;
    }
    case 0x67:
        if (funct3 == 0) instruction->op = OP_JALR;
        instruction->immediate = sign_extend(field(raw, 31, 20), 12);
        break;
    case 0x6f: {
        uint32_t value = (field(raw, 31, 31) << 20) | (field(raw, 19, 12) << 12) |
                         (field(raw, 20, 20) << 11) | (field(raw, 30, 21) << 1);
        instruction->op = OP_JAL;
        instruction->immediate = sign_extend(value, 21);
        break;
    }
    default:
        break;
    }
    return instruction->op != OP_INVALID;
}

const char *op_name(Op op) {
    static const char *const names[] = {
        "invalid", "lb", "lh", "lw", "lbu", "lhu", "addi", "slli", "slti",
        "sltiu", "xori", "srli", "srai", "ori", "andi", "auipc", "lui",
        "sb", "sh", "sw", "add", "sub", "sll", "slt", "sltu", "xor",
        "srl", "sra", "or", "and", "beq", "bne", "blt", "bge", "bltu",
        "bgeu", "jalr", "jal"
    };
    return (unsigned)op < sizeof(names) / sizeof(names[0]) ? names[op] : "invalid";
}

static int64_t signed_immediate(uint32_t value) {
    return (value & 0x80000000u) ? (int64_t)value - 0x100000000LL : (int64_t)value;
}

void format_instruction(const Instruction *i, char *buffer, size_t capacity) {
    int64_t imm = signed_immediate(i->immediate);
    switch (i->op) {
    case OP_LB: case OP_LH: case OP_LW: case OP_LBU: case OP_LHU:
        snprintf(buffer, capacity, "%s x%u, %lld(x%u)", op_name(i->op), i->rd,
                 (long long)imm, i->rs1); break;
    case OP_SB: case OP_SH: case OP_SW:
        snprintf(buffer, capacity, "%s x%u, %lld(x%u)", op_name(i->op), i->rs2,
                 (long long)imm, i->rs1); break;
    case OP_ADDI: case OP_SLTI: case OP_SLTIU: case OP_XORI: case OP_ORI: case OP_ANDI:
        snprintf(buffer, capacity, "%s x%u, x%u, %lld", op_name(i->op), i->rd,
                 i->rs1, (long long)imm); break;
    case OP_SLLI: case OP_SRLI: case OP_SRAI:
        snprintf(buffer, capacity, "%s x%u, x%u, %u", op_name(i->op), i->rd,
                 i->rs1, i->immediate); break;
    case OP_AUIPC: case OP_LUI:
        snprintf(buffer, capacity, "%s x%u, 0x%05X", op_name(i->op), i->rd,
                 (unsigned)(i->immediate >> 12)); break;
    case OP_ADD: case OP_SUB: case OP_SLL: case OP_SLT: case OP_SLTU: case OP_XOR:
    case OP_SRL: case OP_SRA: case OP_OR: case OP_AND:
        snprintf(buffer, capacity, "%s x%u, x%u, x%u", op_name(i->op), i->rd,
                 i->rs1, i->rs2); break;
    case OP_BEQ: case OP_BNE: case OP_BLT: case OP_BGE: case OP_BLTU: case OP_BGEU:
        snprintf(buffer, capacity, "%s x%u, x%u, %lld", op_name(i->op), i->rs1,
                 i->rs2, (long long)imm); break;
    case OP_JALR:
        snprintf(buffer, capacity, "jalr x%u, %lld(x%u)", i->rd, (long long)imm, i->rs1); break;
    case OP_JAL:
        snprintf(buffer, capacity, "jal x%u, %lld", i->rd, (long long)imm); break;
    default:
        snprintf(buffer, capacity, "unknown 0x%08X", i->raw); break;
    }
}
