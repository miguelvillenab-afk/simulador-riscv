#include "memory.h"

#include <stdlib.h>
#include <string.h>

void memory_init(Memory *memory) {
    memory->pages = NULL;
    memory->count = 0;
    memory->capacity = 0;
}

void memory_clear(Memory *memory) {
    for (size_t i = 0; i < memory->count; ++i) free(memory->pages[i].bytes);
    memory->count = 0;
}

void memory_destroy(Memory *memory) {
    memory_clear(memory);
    free(memory->pages);
    memory->pages = NULL;
    memory->capacity = 0;
}

static uint8_t *find_page(const Memory *memory, uint32_t number) {
    for (size_t i = 0; i < memory->count; ++i)
        if (memory->pages[i].number == number) return memory->pages[i].bytes;
    return NULL;
}

static uint8_t *get_or_create_page(Memory *memory, uint32_t number) {
    uint8_t *page = find_page(memory, number);
    if (page != NULL) return page;

    if (memory->count == memory->capacity) {
        size_t next_capacity = memory->capacity == 0 ? 4 : memory->capacity * 2;
        MemoryPage *next = realloc(memory->pages, next_capacity * sizeof(*next));
        if (next == NULL) return NULL;
        memory->pages = next;
        memory->capacity = next_capacity;
    }

    page = calloc(RISCV_PAGE_SIZE, sizeof(*page));
    if (page == NULL) return NULL;
    memory->pages[memory->count].number = number;
    memory->pages[memory->count].bytes = page;
    ++memory->count;
    return page;
}

uint8_t memory_read8(const Memory *memory, uint32_t address) {
    uint8_t *page = find_page(memory, address >> RISCV_PAGE_BITS);
    return page == NULL ? 0 : page[address & (RISCV_PAGE_SIZE - 1u)];
}

uint16_t memory_read16(const Memory *memory, uint32_t address) {
    return (uint16_t)memory_read8(memory, address) |
           (uint16_t)((uint16_t)memory_read8(memory, address + 1u) << 8);
}

uint32_t memory_read32(const Memory *memory, uint32_t address) {
    return (uint32_t)memory_read8(memory, address) |
           ((uint32_t)memory_read8(memory, address + 1u) << 8) |
           ((uint32_t)memory_read8(memory, address + 2u) << 16) |
           ((uint32_t)memory_read8(memory, address + 3u) << 24);
}

bool memory_write8(Memory *memory, uint32_t address, uint8_t value) {
    uint8_t *page = get_or_create_page(memory, address >> RISCV_PAGE_BITS);
    if (page == NULL) return false;
    page[address & (RISCV_PAGE_SIZE - 1u)] = value;
    return true;
}

bool memory_write16(Memory *memory, uint32_t address, uint16_t value) {
    return memory_write8(memory, address, (uint8_t)value) &&
           memory_write8(memory, address + 1u, (uint8_t)(value >> 8));
}

bool memory_write32(Memory *memory, uint32_t address, uint32_t value) {
    return memory_write8(memory, address, (uint8_t)value) &&
           memory_write8(memory, address + 1u, (uint8_t)(value >> 8)) &&
           memory_write8(memory, address + 2u, (uint8_t)(value >> 16)) &&
           memory_write8(memory, address + 3u, (uint8_t)(value >> 24));
}

bool memory_load(Memory *memory, uint32_t address, const uint8_t *bytes, size_t length) {
    for (size_t i = 0; i < length; ++i)
        if (!memory_write8(memory, address + (uint32_t)i, bytes[i])) return false;
    return true;
}
