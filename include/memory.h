#ifndef RISCV_MEMORY_H
#define RISCV_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RISCV_PAGE_BITS 12u
#define RISCV_PAGE_SIZE (1u << RISCV_PAGE_BITS)

typedef struct {
    uint32_t number;
    uint8_t *bytes;
} MemoryPage;

typedef struct {
    MemoryPage *pages;
    size_t count;
    size_t capacity;
} Memory;

void memory_init(Memory *memory);
void memory_destroy(Memory *memory);
void memory_clear(Memory *memory);
uint8_t memory_read8(const Memory *memory, uint32_t address);
uint16_t memory_read16(const Memory *memory, uint32_t address);
uint32_t memory_read32(const Memory *memory, uint32_t address);
bool memory_write8(Memory *memory, uint32_t address, uint8_t value);
bool memory_write16(Memory *memory, uint32_t address, uint16_t value);
bool memory_write32(Memory *memory, uint32_t address, uint32_t value);
bool memory_load(Memory *memory, uint32_t address, const uint8_t *bytes, size_t length);

#endif
